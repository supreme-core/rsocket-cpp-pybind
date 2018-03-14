// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/StreamsFactory.h"

#include "rsocket/statemachine/ChannelRequester.h"
#include "rsocket/statemachine/ChannelResponder.h"
#include "rsocket/statemachine/RSocketStateMachine.h"
#include "rsocket/statemachine/RequestResponseRequester.h"
#include "rsocket/statemachine/RequestResponseResponder.h"
#include "rsocket/statemachine/StreamRequester.h"
#include "rsocket/statemachine/StreamResponder.h"

#include "yarpl/flowable/Flowable.h"
#include "yarpl/single/Singles.h"

namespace rsocket {

using namespace yarpl;

StreamsFactory::StreamsFactory(
    RSocketStateMachine& connection,
    RSocketMode mode)
    : connection_(connection),
      nextStreamId_(
          mode == RSocketMode::CLIENT
              ? 1 /*Streams initiated by a client MUST use
                    odd-numbered stream identifiers*/
              : 2 /*streams initiated by the server MUST use
                    even-numbered stream identifiers*/) {}

static void subscribeToErrorFlowable(
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink) {
  yarpl::flowable::Flowable<Payload>::error(
      std::runtime_error("state machine is disconnected/closed"))
      ->subscribe(std::move(responseSink));
}

static void subscribeToErrorSingle(
    std::shared_ptr<yarpl::single::SingleObserver<Payload>> responseSink) {
  yarpl::single::Singles::error<Payload>(
      std::runtime_error("state machine is disconnected/closed"))
      ->subscribe(std::move(responseSink));
}

std::shared_ptr<yarpl::flowable::Subscriber<Payload>>
StreamsFactory::createChannelRequester(
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink) {
  if (connection_.isDisconnected()) {
    subscribeToErrorFlowable(std::move(responseSink));
    return nullptr;
  }

  auto const streamId = getNextStreamId();
  auto stateMachine = std::make_shared<ChannelRequester>(connection_, streamId);
  connection_.addStream(streamId, stateMachine);
  stateMachine->subscribe(std::move(responseSink));
  return stateMachine;
}

void StreamsFactory::createStreamRequester(
    Payload request,
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink) {
  if (connection_.isDisconnected()) {
    subscribeToErrorFlowable(std::move(responseSink));
    return;
  }

  auto const streamId = getNextStreamId();
  auto stateMachine = std::make_shared<StreamRequester>(
      connection_, streamId, std::move(request));
  connection_.addStream(streamId, stateMachine);
  stateMachine->subscribe(std::move(responseSink));
}

void StreamsFactory::createStreamRequester(
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink,
    StreamId streamId,
    size_t n) {
  if (connection_.isDisconnected()) {
    subscribeToErrorFlowable(std::move(responseSink));
    return;
  }

  auto stateMachine =
      std::make_shared<StreamRequester>(connection_, streamId, Payload());
  // Set requested to true (since cold resumption)
  stateMachine->setRequested(n);
  connection_.addStream(streamId, stateMachine);
  stateMachine->subscribe(std::move(responseSink));
}

void StreamsFactory::createRequestResponseRequester(
    Payload payload,
    std::shared_ptr<yarpl::single::SingleObserver<Payload>> responseSink) {
  if (connection_.isDisconnected()) {
    subscribeToErrorSingle(std::move(responseSink));
    return;
  }

  auto const streamId = getNextStreamId();
  auto stateMachine = std::make_shared<RequestResponseRequester>(
      connection_, streamId, std::move(payload));
  connection_.addStream(streamId, stateMachine);
  stateMachine->subscribe(std::move(responseSink));
}

StreamId StreamsFactory::getNextStreamId() {
  const StreamId streamId = nextStreamId_;
  CHECK(streamId <= std::numeric_limits<int32_t>::max() - 2);
  nextStreamId_ += 2;
  return streamId;
}

void StreamsFactory::setNextStreamId(StreamId streamId) {
  nextStreamId_ = streamId + 2;
}

bool StreamsFactory::registerNewPeerStreamId(StreamId streamId) {
  DCHECK(streamId != 0);
  if (nextStreamId_ % 2 == streamId % 2) {
    // if this is an unknown stream to the socket and this socket is
    // generating
    // such stream ids, it is an incoming frame on the stream which no longer
    // exist
    return false;
  }
  if (streamId <= lastPeerStreamId_) {
    // receiving frame for a stream which no longer exists
    return false;
  }
  lastPeerStreamId_ = streamId;
  return true;
}

std::shared_ptr<ChannelResponder> StreamsFactory::createChannelResponder(
    uint32_t initialRequestN,
    StreamId streamId) {
  auto stateMachine = std::make_shared<ChannelResponder>(
      connection_, streamId, initialRequestN);
  connection_.addStream(streamId, stateMachine);
  return stateMachine;
}

std::shared_ptr<yarpl::flowable::Subscriber<Payload>>
StreamsFactory::createStreamResponder(
    uint32_t initialRequestN,
    StreamId streamId) {
  auto stateMachine =
      std::make_shared<StreamResponder>(connection_, streamId, initialRequestN);
  connection_.addStream(streamId, stateMachine);
  return stateMachine;
}

std::shared_ptr<yarpl::single::SingleObserver<Payload>>
StreamsFactory::createRequestResponseResponder(StreamId streamId) {
  auto stateMachine =
      std::make_shared<RequestResponseResponder>(connection_, streamId);
  connection_.addStream(streamId, stateMachine);
  return stateMachine;
}
} // namespace rsocket
