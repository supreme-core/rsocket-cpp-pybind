// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <glog/logging.h>
#include <iostream>
#include <type_traits>
#include "src/AllowanceSemaphore.h"
#include "src/ConnectionAutomaton.h"
#include "src/Executor.h"
#include "src/Frame.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/RequestHandler.h"

namespace reactivesocket {

enum class StreamCompletionSignal;

/// A class that represents a flow-control-aware producer of data.
class PublisherMixin {
 public:
  explicit PublisherMixin(uint32_t initialRequestN)
      : initialRequestN_(initialRequestN) {}

  /// @{
  void publisherSubscribe(std::shared_ptr<Subscription> subscription) {
    debugCheckOnSubscribe();
    producingSubscription_ = std::move(subscription);
    if (initialRequestN_) {
      producingSubscription_->request(initialRequestN_.drain());
    }
  }

  /// @}

  std::shared_ptr<Subscription> subscription() const {
    return producingSubscription_;
  }

  void processRequestN(uint32_t requestN) {
    if (!requestN) {
      return;
    }

    // we might not have the subscription set yet as there can be REQUEST_N
    // frames scheduled on the executor before onSubscribe method
    if (producingSubscription_) {
      producingSubscription_->request(requestN);
    } else {
      initialRequestN_.release(requestN);
    }
  }

 protected:
  void debugCheckOnSubscribe() {
    DCHECK(!producingSubscription_);
  }

  void debugCheckOnNextOnCompleteOnError() {
    DCHECK(producingSubscription_);
  }

  /// @{
  void terminatePublisher(StreamCompletionSignal signal) {
    if (auto subscription = std::move(producingSubscription_)) {
      subscription->cancel();
    }
  }

  void pausePublisherStream(RequestHandler& requestHandler) {
    if (producingSubscription_) {
      requestHandler.onSubscriptionPaused(producingSubscription_);
    }
  }

  void resumePublisherStream(RequestHandler& requestHandler) {
    if (producingSubscription_) {
      requestHandler.onSubscriptionResumed(producingSubscription_);
    }
  }

 private:
  /// A Subscription that constrols production of payloads.
  /// This mixin is responsible for delivering a terminal signal to the
  /// Subscription once the stream ends.
  std::shared_ptr<Subscription> producingSubscription_;
  AllowanceSemaphore initialRequestN_;
};
}
