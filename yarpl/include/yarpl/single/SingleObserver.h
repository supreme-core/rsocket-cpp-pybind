// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Refcounted.h"
#include "yarpl/single/SingleSubscription.h"

#include <folly/ExceptionWrapper.h>

#include <glog/logging.h>

namespace yarpl {
namespace single {

template <typename T>
class SingleObserver : public virtual Refcounted, public yarpl::enable_get_ref {
public:
  virtual void onSubscribe(std::shared_ptr<SingleSubscription>) = 0;
  virtual void onSuccess(T) = 0;
  virtual void onError(folly::exception_wrapper) = 0;
};

template <typename T>
class SingleObserverBase : public SingleObserver<T> {
 public:
  // Note: If any of the following methods is overridden in a subclass, the new
  // methods SHOULD ensure that these are invoked as well.
  void onSubscribe(std::shared_ptr<SingleSubscription> subscription) override {
    DCHECK(subscription);

    if (subscription_) {
      subscription->cancel();
      return;
    }

    subscription_ = std::move(subscription);
  }

  void onSuccess(T) override {
    DCHECK(subscription_) << "Calling onSuccess() without a subscription";
    subscription_.reset();
  }

  // No further calls to the subscription after this method is invoked.
  void onError(folly::exception_wrapper) override {
    DCHECK(subscription_) << "Calling onError() without a subscription";
    subscription_.reset();
  }

 protected:
  SingleSubscription* subscription() {
    return subscription_.operator->();
  }

 private:
  std::shared_ptr<SingleSubscription> subscription_;
};

/// Specialization of SingleObserverBase<void>.
template <>
class SingleObserverBase<void> : public virtual Refcounted {
 public:
  // Note: If any of the following methods is overridden in a subclass, the new
  // methods SHOULD ensure that these are invoked as well.
  virtual void onSubscribe(std::shared_ptr<SingleSubscription> subscription) {
    DCHECK(subscription);

    if (subscription_) {
      subscription->cancel();
      return;
    }

    subscription_ = std::move(subscription);
  }

  virtual void onSuccess() {
    DCHECK(subscription_) << "Calling onSuccess() without a subscription";
    subscription_.reset();
  }

  // No further calls to the subscription after this method is invoked.
  virtual void onError(folly::exception_wrapper) {
    DCHECK(subscription_) << "Calling onError() without a subscription";
    subscription_.reset();
  }

 protected:
  SingleSubscription* subscription() {
    return subscription_.operator->();
  }

 private:
  std::shared_ptr<SingleSubscription> subscription_;
};
}
}
