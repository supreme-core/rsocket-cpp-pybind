// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/RequestHandler.h"

namespace reactivesocket {

class ReactiveSocketSubscriberFactory : public SubscriberFactory {
 public:
  using TFactoryCallback =
      std::function<Subscriber<Payload>&(folly::Executor*)>;

  ReactiveSocketSubscriberFactory(TFactoryCallback factoryCallback)
      : factoryCallback_(std::move(factoryCallback)) {}

  Subscriber<Payload>& createSubscriber() override {
    CHECK(factoryCallback_);
    auto call = std::move(factoryCallback_);
    return call(nullptr);
  }

  Subscriber<Payload>& createSubscriber(folly::Executor& executor) override {
    CHECK(factoryCallback_);
    auto call = std::move(factoryCallback_);
    return call(&executor);
  }

 private:
  TFactoryCallback factoryCallback_;
};

} // reactivesocket
