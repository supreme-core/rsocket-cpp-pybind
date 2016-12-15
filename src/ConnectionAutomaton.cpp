// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/ConnectionAutomaton.h"

#include <folly/ExceptionWrapper.h>
#include <folly/String.h>
#include "src/AbstractStreamAutomaton.h"
#include "src/ClientResumeStatusCallback.h"

namespace reactivesocket {

ConnectionAutomaton::ConnectionAutomaton(
    std::unique_ptr<DuplexConnection> connection,
    StreamAutomatonFactory factory,
    std::shared_ptr<StreamState> streamState,
    ResumeListener resumeListener,
    Stats& stats,
    const std::shared_ptr<KeepaliveTimer>& keepaliveTimer,
    bool isServer,
    bool isResumable,
    std::function<void()> onConnected,
    std::function<void()> onDisconnected,
    std::function<void()> onClosed)
    : connection_(std::move(connection)),
      factory_(std::move(factory)),
      streamState_(std::move(streamState)),
      stats_(stats),
      isServer_(isServer),
      isResumable_(isResumable),
      onConnected_(std::move(onConnected)),
      onDisconnected_(std::move(onDisconnected)),
      onClosed_(std::move(onClosed)),
      resumeListener_(resumeListener),
      keepaliveTimer_(keepaliveTimer) {
  // We deliberately do not "open" input or output to avoid having c'tor on the
  // stack when processing any signals from the connection. See ::connect and
  // ::onSubscribe.
  CHECK(connection_);
  CHECK(streamState_);
  CHECK(onConnected_);
  CHECK(onDisconnected_);
  CHECK(onClosed_);
}

ConnectionAutomaton::~ConnectionAutomaton() {
  VLOG(6) << "~ConnectionAutomaton";
  // We rely on SubscriptionPtr and SubscriberPtr to dispatch appropriate
  // terminal signals.
}

void ConnectionAutomaton::connect() {
  CHECK(connection_);
  connectionOutput_.reset(connection_->getOutput());
  connectionOutput_.onSubscribe(shared_from_this());

  // the onSubscribe call on the previous line may have called the terminating
  // signal which would call disconnect/close
  if (connection_) {
    // This may call ::onSubscribe in-line, which calls ::request on the
    // provided
    // subscription, which might deliver frames in-line.
    // it can also call onComplete which will call disconnect/close and reset
    // the connection_ while still inside of the connection_::setInput method.
    // We will create a hard reference for that case and keep the object alive
    // until setInput method returns
    auto connectionCopy = connection_;
    connectionCopy->setInput(shared_from_this());
  }

  if (connection_) {
    onConnected_();
  }

  // TODO: move to appropriate place
  stats_.socketCreated();
}

void ConnectionAutomaton::disconnect() {
  VLOG(6) << "disconnect";
  if (!connection_) {
    return;
  }

  closeDuplexConnection(folly::exception_wrapper());
  stats_.socketDisconnected();
  onDisconnected_();
}

void ConnectionAutomaton::close() {
  close(folly::exception_wrapper(), StreamCompletionSignal::SOCKET_CLOSED);
}

void ConnectionAutomaton::close(
    folly::exception_wrapper ex,
    StreamCompletionSignal signal) {
  VLOG(6) << "close";
  closeStreams(signal);
  closeDuplexConnection(std::move(ex));
  if (onClosed_) {
    stats_.socketClosed();
    onClosed_();
    onClosed_ = nullptr;
  }
}

void ConnectionAutomaton::closeDuplexConnection(folly::exception_wrapper ex) {
  if (!connection_) {
    return;
  }

  auto oldConnection = std::move(connection_);

  // Send terminal signals to the DuplexConnection's input and output before
  // tearing it down. We must do this per DuplexConnection specification (see
  // interface definition).
  if (ex) {
    connectionOutput_.onError(std::move(ex));
  } else {
    connectionOutput_.onComplete();
  }
  connectionInputSub_.cancel();
}

void ConnectionAutomaton::closeWithError(Frame_ERROR&& error) {
  VLOG(4) << "closeWithError "
          << error.payload_.data->cloneAsValue().moveToFbString();

  outputFrameOrEnqueue(error.serializeOut());
  close(folly::exception_wrapper(), StreamCompletionSignal::ERROR);
}

void ConnectionAutomaton::reconnect(
    std::unique_ptr<DuplexConnection> newConnection,
    std::unique_ptr<ClientResumeStatusCallback> resumeCallback) {
  // TODO(lehecka)
  if (resumeCallback) {
    resumeCallback->onResumeError(std::runtime_error("not implemented"));
  }
  disconnect();
  connection_ = std::shared_ptr<DuplexConnection>(std::move(newConnection));
  connect();
}

void ConnectionAutomaton::addStream(
    StreamId streamId,
    std::shared_ptr<AbstractStreamAutomaton> automaton) {
  auto result = streamState_->streams_.emplace(streamId, std::move(automaton));
  (void)result;
  assert(result.second);
}

void ConnectionAutomaton::endStream(
    StreamId streamId,
    StreamCompletionSignal signal) {
  VLOG(6) << "endStream";
  // The signal must be idempotent.
  if (!endStreamInternal(streamId, signal)) {
    return;
  }
  // TODO(stupaq): handle connection-level errors
  assert(
      signal == StreamCompletionSignal::GRACEFUL ||
      signal == StreamCompletionSignal::ERROR);
}

bool ConnectionAutomaton::endStreamInternal(
    StreamId streamId,
    StreamCompletionSignal signal) {
  VLOG(6) << "endStreamInternal";
  auto it = streamState_->streams_.find(streamId);
  if (it == streamState_->streams_.end()) {
    // Unsubscribe handshake initiated by the connection, we're done.
    return false;
  }
  // Remove from the map before notifying the automaton.
  auto automaton = std::move(it->second);
  streamState_->streams_.erase(it);
  automaton->endStream(signal);
  return true;
}

void ConnectionAutomaton::closeStreams(StreamCompletionSignal signal) {
  // Close all streams.
  while (!streamState_->streams_.empty()) {
    auto oldSize = streamState_->streams_.size();
    auto result =
        endStreamInternal(streamState_->streams_.begin()->first, signal);
    (void)oldSize;
    (void)result;
    // TODO(stupaq): what kind of a user action could violate these
    // assertions?
    assert(result);
    assert(streamState_->streams_.size() == oldSize - 1);
  }
}

/// @{
void ConnectionAutomaton::onSubscribe(
    std::shared_ptr<Subscription> subscription) {
  assert(!connectionInputSub_);
  connectionInputSub_.reset(std::move(subscription));
  // This may result in signals being issued by the connection in-line, see
  // ::connect.
  connectionInputSub_.request(std::numeric_limits<size_t>::max());
}

void ConnectionAutomaton::onNext(std::unique_ptr<folly::IOBuf> frame) {
  auto frameType = FrameHeader::peekType(*frame);

  std::stringstream ss;
  ss << frameType;

  stats_.frameRead(ss.str());

  // TODO(tmont): If a frame is invalid, it will still be tracked. However, we
  // actually want that. We want to keep
  // each side in sync, even if a frame is invalid.
  streamState_->resumeTracker_->trackReceivedFrame(*frame);

  auto streamIdPtr = FrameHeader::peekStreamId(*frame);
  if (!streamIdPtr) {
    // Failed to deserialize the frame.
    closeWithError(Frame_ERROR::connectionError("invalid frame"));
    return;
  }
  auto streamId = *streamIdPtr;
  if (streamId == 0) {
    onConnectionFrame(std::move(frame));
    return;
  }
  auto it = streamState_->streams_.find(streamId);
  if (it == streamState_->streams_.end()) {
    handleUnknownStream(streamId, std::move(frame));
    return;
  }
  auto automaton = it->second;
  // Can deliver the frame.
  automaton->onNextFrame(std::move(frame));
}

void ConnectionAutomaton::onDuplexConnectionTerminal(
    folly::exception_wrapper ex,
    StreamCompletionSignal signal) {
  if (isResumable_) {
    disconnect();
  } else {
    close(std::move(ex), signal);
  }
}

void ConnectionAutomaton::onComplete() {
  VLOG(6) << "onComplete";
  onDuplexConnectionTerminal(
      folly::exception_wrapper(), StreamCompletionSignal::CONNECTION_END);
}

void ConnectionAutomaton::onError(folly::exception_wrapper ex) {
  VLOG(6) << "onError" << ex.what();
  onDuplexConnectionTerminal(
      std::move(ex), StreamCompletionSignal::CONNECTION_ERROR);
}

void ConnectionAutomaton::onConnectionFrame(
    std::unique_ptr<folly::IOBuf> payload) {
  auto type = FrameHeader::peekType(*payload);
  switch (type) {
    case FrameType::KEEPALIVE: {
      Frame_KEEPALIVE frame;
      if (frame.deserializeFrom(std::move(payload))) {
        if (isServer_) {
          if (frame.header_.flags_ & FrameFlags_KEEPALIVE_RESPOND) {
            frame.header_.flags_ &= ~(FrameFlags_KEEPALIVE_RESPOND);
            outputFrameOrEnqueue(frame.serializeOut());
          } else {
            closeWithError(
                Frame_ERROR::connectionError("keepalive without flag"));
          }

          streamState_->resumeCache_->resetUpToPosition(frame.position_);
        } else {
          if (frame.header_.flags_ & FrameFlags_KEEPALIVE_RESPOND) {
            closeWithError(Frame_ERROR::connectionError(
                "client received keepalive with respond flag"));
          } else if (keepaliveTimer_) {
            keepaliveTimer_->keepaliveReceived();
          }
        }
      } else {
        closeWithError(Frame_ERROR::unexpectedFrame());
      }
      return;
    }
    case FrameType::SETUP: {
      // TODO(tmont): check for ENABLE_RESUME and make sure isResumable_ is true
      if (!factory_(*this, 0, std::move(payload))) {
        assert(false);
      }
      return;
    }
    case FrameType::METADATA_PUSH: {
      if (!factory_(*this, 0, std::move(payload))) {
        assert(false);
      }
      return;
    }
    case FrameType::RESUME: {
      Frame_RESUME frame;
      if (frame.deserializeFrom(std::move(payload))) {
        bool canResume = false;

        if (isServer_ && isResumable_) {
          auto streamState = resumeListener_(frame.token_);
          if (nullptr != streamState) {
            canResume = true;
            useStreamState(streamState);
          }
        }

        if (canResume) {
          outputFrameOrEnqueue(
              Frame_RESUME_OK(streamState_->resumeTracker_->impliedPosition())
                  .serializeOut());
          for (auto it : streamState_->streams_) {
            const StreamId streamId = it.first;

            if (streamState_->resumeCache_->isPositionAvailable(
                    frame.position_, streamId)) {
              it.second->onCleanResume();
            } else {
              it.second->onDirtyResume();
            }
          }
        } else {
          closeWithError(Frame_ERROR::connectionError("can not resume"));
        }
      } else {
        closeWithError(Frame_ERROR::unexpectedFrame());
      }
      return;
    }
    case FrameType::RESUME_OK: {
      Frame_RESUME_OK frame;
      if (frame.deserializeFrom(std::move(payload))) {
        if (!isServer_ && isResumable_ &&
            streamState_->resumeCache_->isPositionAvailable(frame.position_)) {
        } else {
          closeWithError(Frame_ERROR::connectionError("can not resume"));
        }
      } else {
        closeWithError(Frame_ERROR::unexpectedFrame());
      }
      return;
    }
    default:
      closeWithError(Frame_ERROR::unexpectedFrame());
      return;
  }
}
/// @}

void ConnectionAutomaton::request(size_t n) {
  if (writeAllowance_.release(n) > 0) {
    // There are no pending writes or we already have this method on the
    // stack.
    return;
  }
  drainOutputFramesQueue();
}

void ConnectionAutomaton::cancel() {
  VLOG(6) << "cancel";
  onDuplexConnectionTerminal(
      folly::exception_wrapper(), StreamCompletionSignal::CONNECTION_END);
}

/// @{
void ConnectionAutomaton::handleUnknownStream(
    StreamId streamId,
    std::unique_ptr<folly::IOBuf> payload) {
  // TODO(stupaq): there are some rules about monotonically increasing stream
  // IDs -- let's forget about them for a moment
  if (!factory_(*this, streamId, std::move(payload))) {
    closeWithError(Frame_ERROR::connectionError(
        folly::to<std::string>("unknown stream ", streamId)));
  }
}
/// @}

void ConnectionAutomaton::sendKeepalive() {
  Frame_KEEPALIVE pingFrame(
      FrameFlags_KEEPALIVE_RESPOND,
      streamState_->resumeTracker_->impliedPosition(),
      folly::IOBuf::create(0));
  outputFrameOrEnqueue(pingFrame.serializeOut());
}

void ConnectionAutomaton::sendResume(const ResumeIdentificationToken& token) {
  Frame_RESUME resumeFrame(
      token, streamState_->resumeTracker_->impliedPosition());
  outputFrameOrEnqueue(resumeFrame.serializeOut());
}

bool ConnectionAutomaton::isPositionAvailable(ResumePosition position) {
  return streamState_->resumeCache_->isPositionAvailable(position);
}

ResumePosition ConnectionAutomaton::positionDifference(
    ResumePosition position) {
  return streamState_->resumeCache_->position() - position;
}

void ConnectionAutomaton::outputFrameOrEnqueue(
    std::unique_ptr<folly::IOBuf> frame) {
  if (connection_) {
    drainOutputFramesQueue();
    if (streamState_->pendingWrites_.empty() && writeAllowance_.tryAcquire()) {
      outputFrame(std::move(frame));
      return;
    }
  }
  // We either have no allowance to perform the operation, or the queue has
  // not been drained (e.g. we're looping in ::request).
  // or we are disconnected
  streamState_->pendingWrites_.emplace_back(std::move(frame));
}

void ConnectionAutomaton::drainOutputFramesQueue() {
  // Drain the queue or the allowance.
  while (!streamState_->pendingWrites_.empty() &&
         writeAllowance_.tryAcquire()) {
    auto frame = std::move(streamState_->pendingWrites_.front());
    streamState_->pendingWrites_.pop_front();
    outputFrame(std::move(frame));
  }
}

void ConnectionAutomaton::outputFrame(
    std::unique_ptr<folly::IOBuf> outputFrame) {
  std::stringstream ss;
  ss << FrameHeader::peekType(*outputFrame);

  stats_.frameWritten(ss.str());

  streamState_->resumeCache_->trackSentFrame(*outputFrame);
  connectionOutput_.onNext(std::move(outputFrame));
}

void ConnectionAutomaton::useStreamState(
    std::shared_ptr<StreamState> streamState) {
  CHECK(streamState);
  if (isServer_ && isResumable_) {
    streamState_.swap(streamState);
  }
}
}
