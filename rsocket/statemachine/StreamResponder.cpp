// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/StreamResponder.h"

namespace rsocket {

void StreamResponder::onSubscribe(
    std::shared_ptr<yarpl::flowable::Subscription> subscription) {
  publisherSubscribe(std::move(subscription));
}

void StreamResponder::onNext(Payload response) {
  if (!publisherClosed()) {
    writePayload(std::move(response));
  }
}

void StreamResponder::onComplete() {
  if (!publisherClosed()) {
    publisherComplete();
    writeComplete();
    removeFromWriter();
  }
}

void StreamResponder::onError(folly::exception_wrapper ex) {
  if (!publisherClosed()) {
    publisherComplete();
    writeApplicationError(ex.get_exception()->what());
    removeFromWriter();
  }
}

void StreamResponder::endStream(StreamCompletionSignal) {
  terminatePublisher();
}

void StreamResponder::handleCancel() {
  terminatePublisher();
  removeFromWriter();
}

void StreamResponder::handleRequestN(uint32_t n) {
  processRequestN(n);
}

} // namespace rsocket
