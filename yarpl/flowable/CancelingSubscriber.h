// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/flowable/Subscriber.h"

#include <stdexcept>

namespace yarpl {
namespace flowable {

/**
 * A Subscriber that always cancels the subscription passed to it.
 */
template <typename T>
class CancelingSubscriber final : public BaseSubscriber<T> {
 public:
  void onSubscribeImpl() override {
    this->cancel();
  }

  void onNextImpl(T) override {
    throw std::logic_error{"CancelingSubscriber::onNext() can never be called"};
  }
  void onCompleteImpl() override {
    throw std::logic_error{"CancelingSubscriber::onComplete() can never be called"};
  }
  void onErrorImpl(folly::exception_wrapper) override {
    throw std::logic_error{"CancelingSubscriber::onError() can never be called"};
  }
};
}
}
