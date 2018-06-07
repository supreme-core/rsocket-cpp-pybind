// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/StreamResponder.h"

namespace rsocket {

void StreamResponder::onSubscribe(
    std::shared_ptr<yarpl::flowable::Subscription> subscription) {
  publisherSubscribe(std::move(subscription));
}

void StreamResponder::onNext(Payload response) {
  if (publisherClosed()) {
    return;
  }
  writePayload(std::move(response));
}

void StreamResponder::onComplete() {
  if (publisherClosed()) {
    return;
  }
  publisherComplete();
  writeComplete();
  removeFromWriter();
}

void StreamResponder::onError(folly::exception_wrapper ew) {
  if (publisherClosed()) {
    return;
  }
  publisherComplete();
  writeApplicationError(ew.get_exception()->what());
  removeFromWriter();
}

void StreamResponder::handleRequestN(uint32_t n) {
  processRequestN(n);
}

void StreamResponder::handleError(folly::exception_wrapper) {
  handleCancel();
}

void StreamResponder::handlePayload(
    Payload&& payload,
    bool /*flagsComplete*/,
    bool /*flagsNext*/,
    bool flagsFollows) {
  payloadFragments_.addPayloadIgnoreFlags(std::move(payload));

  if (flagsFollows) {
    // there will be more fragments to come
    return;
  }

  Payload finalPayload = payloadFragments_.consumePayloadIgnoreFlags();

  if (newStream_) {
    newStream_ = false;
    onNewStreamReady(
        StreamType::STREAM, std::move(finalPayload), shared_from_this());
  } else {
    // per rsocket spec, ignore unexpected frame (payload) if it makes no sense
    // in the semantic of the stream
  }
}

void StreamResponder::handleCancel() {
  if (publisherClosed()) {
    return;
  }
  terminatePublisher();
  removeFromWriter();
}

void StreamResponder::endStream(StreamCompletionSignal signal) {
  if (publisherClosed()) {
    return;
  }
  terminatePublisher();
  writeApplicationError(to_string(signal));
  removeFromWriter();
}

} // namespace rsocket
