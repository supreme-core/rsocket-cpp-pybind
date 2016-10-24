// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <reactive-streams/utilities/SmartPointers.h>
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/mixins/ExecutorMixin.h"

namespace reactivesocket {

class CancellingSubscription : public SubscriptionBase {
 public:
  explicit CancellingSubscription(
      std::shared_ptr<Subscriber<Payload>> subscriber)
      : subscriber_(std::move(subscriber)) {}

 private:
  void requestImpl(size_t n) override {
    subscriber_.onComplete();
  }

  void cancelImpl() override {
    subscriber_.onComplete();
  }

  SubscriberPtr<Subscriber<Payload>> subscriber_;
};

} // reactivesocket
