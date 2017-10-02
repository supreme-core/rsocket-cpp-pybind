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

// T : Type of Flowable that this Subscriber operates on
//
// keep_reference_to_this : BaseSubscriber will keep a live reference to
// itself on the stack while in a signaling or requesting method, in case
// the derived class causes all other references to itself to be dropped.
//
// Classes that ensure that at least one reference will stay live can
// use `keep_reference_to_this = false` as an optimization to
// prevent an atomic inc/dec pair
template <typename T>
class BaseSubscriber : public Subscriber<T> {
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
    if(auto sub = subscription_.exchange(nullptr)) {
      onCompleteImpl();
      onTerminateImpl();
    }
  }

  // No further calls to the subscription after this method is invoked.
  void onError(folly::exception_wrapper e) final override {
    if(auto sub = subscription_.exchange(nullptr)) {
      onErrorImpl(std::move(e));
      onTerminateImpl();
    }
  }

  void onNext(T t) final override {
    if(auto sub = subscription_.load()) {
      onNextImpl(std::move(t));
    }
  }

  void cancel() {
    if(auto sub = subscription_.exchange(nullptr)) {
      sub->cancel();
      onTerminateImpl();
    }
  }

  void request(int64_t n) {
    if(auto sub = subscription_.load()) {
      sub->request(n);
    }
  }

  virtual void onSubscribeImpl() = 0;
  virtual void onCompleteImpl() = 0;
  virtual void onNextImpl(T) = 0;
  virtual void onErrorImpl(folly::exception_wrapper) = 0;

  virtual void onTerminateImpl() {}

 private:
  // keeps a reference alive to the subscription
  AtomicReference<Subscription> subscription_;
};

}
} /* namespace yarpl::flowable */
