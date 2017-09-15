// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Refcounted.h"
#include "yarpl/flowable/Subscription.h"

#include <folly/ExceptionWrapper.h>

#include <glog/logging.h>

namespace yarpl {
namespace flowable {

template <typename T>
class Subscriber : public virtual Refcounted, public yarpl::enable_get_ref {
 public:
  virtual void onSubscribe(Reference<Subscription>) = 0;
  virtual void onComplete() = 0;
  virtual void onError(folly::exception_wrapper) = 0;
  virtual void onNext(T) = 0;
};

// codemod all things that inherit from Subscriber to inherit from LegacySubscriber
template <typename T>
class LegacySubscriber : public Subscriber<T> {
 public:
  // Note: If any of the following methods is overridden in a subclass, the new
  // methods SHOULD ensure that these are invoked as well.
  void onSubscribe(Reference<Subscription> subscription) override {
    DCHECK(subscription);
    CHECK(!subscription_);
    subscription_ = std::move(subscription);
  }

  // No further calls to the subscription after this method is invoked.
  void onComplete() override {
    DCHECK(subscription_) << "Calling onComplete() without a subscription";
    subscription_.reset();
  }

  // No further calls to the subscription after this method is invoked.
  void onError(folly::exception_wrapper) override {
    DCHECK(subscription_) << "Calling onError() without a subscription";
    subscription_.reset();
  }

 protected:
  Reference<Subscription> subscription() {
    return subscription_;
  }

 private:
  Reference<Subscription> subscription_;
};

template <typename T>
class SafeSubscriber : public Subscriber<T> {
 public:
  // Note: If any of the following methods is overridden in a subclass, the new
  // methods SHOULD ensure that these are invoked as well.
  void onSubscribe(Reference<Subscription> subscription) final override {
    DCHECK(subscription);
    CHECK(!subscription_);
    subscription_ = std::move(subscription);
    onSubscribeImpl();
  }

  // No further calls to the subscription after this method is invoked.
  void onComplete() final override {
    DCHECK(subscription_) << "Calling onComplete() without a subscription";
    subscription_.reset();
    onCompleteImpl();
  }

  // No further calls to the subscription after this method is invoked.
  void onError(folly::exception_wrapper e) final override {
    DCHECK(subscription_) << "Calling onError() without a subscription";
    subscription_.reset();
    onErrorImpl(std::move(e));
  }

  void onNext(T t) final override {
    onNextImpl(std::move(t));
  }

  virtual void onSubscribeImpl() = 0;
  virtual void onCompleteImpl() = 0;
  virtual void onNextImpl(T) = 0;
  virtual void onErrorImpl(folly::exception_wrapper) = 0;

 protected:
  Reference<Subscription> subscription() {
    return subscription_;
  }

 private:
  Reference<Subscription> subscription_;
};

template <typename T>
class SafeDefaultSubscriber : public SafeSubscriber<T> {
  void onSubscribeImpl() override {}
  void onNextImpl(T) override {}
  void onCompleteImpl() override {}
  void onErrorImpl(folly::exception_wrapper) override {}
};

} } /* namespace yarpl::flowable */
