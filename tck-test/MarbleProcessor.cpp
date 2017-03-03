// Copyright 2004-present Facebook. All Rights Reserved.

#include "MarbleProcessor.h"

#include <algorithm>

#include <folly/Conv.h>
#include <folly/ExceptionWrapper.h>

#include "src/Payload.h"

namespace reactivesocket {
namespace tck {

MarbleProcessor::MarbleProcessor(
    const std::string marble,
    const std::shared_ptr<Subscriber<Payload>>& subscriber)
    : marble_(std::move(marble)), subscriber_(subscriber.get()) {
  // Remove '-' which is of no consequence for the tests
  marble_.erase(
      std::remove(marble_.begin(), marble_.end(), '-'), marble_.end());
}

void MarbleProcessor::run() {
  for (const char& c : marble_) {
    if (terminated_) {
      return;
    }
    switch (c) {
      case '#':
        while (!canTerminate_)
          ;
        LOG(INFO) << "Sending onError";
        subscriber_->onError(std::runtime_error("Marble Triggered Error"));
        return;
      case '|':
        while (!canTerminate_)
          ;
        LOG(INFO) << "Sending onComplete";
        subscriber_->onComplete();
        return;
      default:
        while (canSend_ <= 0)
          ;
        LOG(INFO) << "Sending data " << c;
        subscriber_->onNext(Payload(folly::to<std::string>(c)));
        canSend_--;
        break;
    }
  }
}

void MarbleProcessor::request(size_t n) {
  canTerminate_ = true;
  canSend_ += n;
}

void MarbleProcessor::cancel() {
  terminated_ = true;
}

} // tck
} // reactivesocket
