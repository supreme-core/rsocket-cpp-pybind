// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include <folly/MoveWrapper.h>
#include <glog/logging.h>
#include "src/EnableSharedFromThis.h"
#include "src/Executor.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"

namespace reactivesocket {

//
// SubscriberBase is designed to be a general purpose base class for
// implementors of Subscriber<T> interface. It provides 2 key features:
// 1. ExecutorBase: You can specify the Executor you want to use to execute the
//    on{Subscribe,Next,Complete,Error} methods. Eq. by providing EventBase
//    as an executor, you can call the methods from any thread and the
//    marshaling to the right EventBase thread will happen automatically.
// 2. once the user cancels the subscriber via the subscription::cancel, it is
//    guaranteed that no more callbacks will be called. It is safe for the
// client to do cleanup and releasing of resources. User doesn't have to call
//    subscription::cancel in on{Complete,Error} methods.
template <typename T>
class SubscriberBaseT : public Subscriber<T>,
                        public EnableSharedFromThisBase<SubscriberBaseT<T>>,
                        public virtual ExecutorBase {
  virtual void onSubscribeImpl(std::shared_ptr<Subscription> subscription) = 0;
  virtual void onNextImpl(T payload) = 0;
  virtual void onCompleteImpl() = 0;
  virtual void onErrorImpl(folly::exception_wrapper ex) = 0;

  // used to be able to cancel subscription immediately
  // also to break the reference cycle involving storing subscription pointer
  // in the derived class from the SubscriberBase
  class SubscriptionShim : public Subscription {
   public:
    explicit SubscriptionShim(
        std::shared_ptr<Subscription> originalSubscription)
        : cancelled_(false),
          originalSubscription_(std::move(originalSubscription)) {
      DCHECK(originalSubscription_);
    }

    void request(size_t n) override final {
      // the original subscription is responsible to handle receiving
      // request(n) after cancel in the case it happens
      if (auto subscription = std::atomic_load(&originalSubscription_)) {
        subscription->request(n);
      }
    }

    void cancel() override final {
      if (!cancelled_.exchange(true)) {
        cancelOriginalSubscription();
      }
    }

    void cancelOriginalSubscription() {
      if (auto subscription = std::atomic_exchange(
              &originalSubscription_, std::shared_ptr<Subscription>())) {
        subscription->cancel();
      }
    }

    std::atomic<bool>& cancelledAtomic() {
      return cancelled_;
    }

   private:
    std::atomic<bool> cancelled_;
    std::shared_ptr<Subscription> originalSubscription_;
  };

 public:
  explicit SubscriberBaseT(folly::Executor& executor = defaultExecutor())
      : ExecutorBase(executor) {}

  void onSubscribe(std::shared_ptr<Subscription> subscription) override final {
    auto thisPtr = this->shared_from_this();
    runInExecutor([thisPtr, subscription]() {
      VLOG(1) << (ExecutorBase*)thisPtr.get() << " onSubscribe";
      CHECK(!thisPtr->subscriptionShim_);
      thisPtr->subscriptionShim_ =
          std::make_shared<SubscriptionShim>(std::move(subscription));
      thisPtr->onSubscribeImpl(thisPtr->subscriptionShim_);
    });
  }

  void onNext(T payload) override final {
    // Scheduling the calls is not atomic operation so it may very well happen
    // that 2 threads race for sending onNext and onComplete. We need to make
    // sure
    // that once the terminating signal is delivered we no longer try to deliver
    // onNext.
    auto movedPayload = folly::makeMoveWrapper(std::move(payload));
    auto thisPtr = this->shared_from_this();
    runInExecutor([thisPtr, movedPayload]() mutable {
      VLOG(1) << (ExecutorBase*)thisPtr.get() << " onNext";
      if (!thisPtr->isCancelled()) {
        thisPtr->onNextImpl(movedPayload.move());
      }
    });
  }

  void onComplete() override final {
    auto thisPtr = this->shared_from_this();
    runInExecutor([thisPtr]() {
      VLOG(1) << (ExecutorBase*)thisPtr.get() << " onComplete";
      if (thisPtr->setCancelled()) {
        thisPtr->onCompleteImpl();
        thisPtr->subscriptionShim_->cancelOriginalSubscription();
        thisPtr->subscriptionShim_ = nullptr;
      }
    });
  }

  void onError(folly::exception_wrapper ex) override final {
    auto movedEx = folly::makeMoveWrapper(std::move(ex));
    auto thisPtr = this->shared_from_this();
    runInExecutor([thisPtr, movedEx]() mutable {
      VLOG(1) << (ExecutorBase*)thisPtr.get() << " onError";
      if (thisPtr->setCancelled()) {
        thisPtr->onErrorImpl(movedEx.move());
        thisPtr->subscriptionShim_->cancelOriginalSubscription();
        thisPtr->subscriptionShim_ = nullptr;
      }
    });
  }

 protected:
  bool isCancelled() const {
    return !subscriptionShim_ || subscriptionShim_->cancelledAtomic();
  }

 private:
  bool setCancelled() {
    return subscriptionShim_ &&
        !subscriptionShim_->cancelledAtomic().exchange(true);
  }

  std::shared_ptr<SubscriptionShim> subscriptionShim_;
};

extern template class SubscriberBaseT<Payload>;
extern template class SubscriberBaseT<folly::IOBuf>;

using SubscriberBase = SubscriberBaseT<Payload>;

} // reactivesocket
