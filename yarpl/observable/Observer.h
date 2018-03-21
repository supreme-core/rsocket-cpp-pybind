// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include <folly/functional/Invoke.h>
#include <glog/logging.h>
#include "yarpl/Refcounted.h"
#include "yarpl/observable/Subscription.h"

namespace yarpl {
namespace observable {

template <typename T>
class Observer : public yarpl::enable_get_ref {
 public:
  // Note: If any of the following methods is overridden in a subclass, the new
  // methods SHOULD ensure that these are invoked as well.
  virtual void onSubscribe(std::shared_ptr<Subscription> subscription) {
    DCHECK(subscription);

    if (subscription_) {
      DLOG(ERROR) << "attempt to double subscribe";
      subscription->cancel();
      return;
    }

    subscription_ = std::move(subscription);
  }

  // No further calls to the subscription after this method is invoked.
  virtual void onComplete() {
    DCHECK(subscription_) << "Calling onComplete() without a subscription";
    subscription_.reset();
  }

  // No further calls to the subscription after this method is invoked.
  virtual void onError(folly::exception_wrapper) {
    DCHECK(subscription_) << "Calling onError() without a subscription";
    subscription_.reset();
  }

  virtual void onNext(T) = 0;

  bool isUnsubscribed() const {
    CHECK(subscription_);
    return subscription_->isCancelled();
  }

  // Ability to add more subscription objects which will be notified when the
  // subscription has been cancelled.
  // Note that calling cancel on the tied subscription is not going to cancel
  // this subscriber
  void addSubscription(std::shared_ptr<Subscription> subscription) {
    if (!subscription_) {
      subscription->cancel();
      return;
    }
    subscription_->tieSubscription(std::move(subscription));
  }

  template <typename OnCancel>
  void addSubscription(OnCancel onCancel) {
    addSubscription(Subscription::create(std::move(onCancel)));
  }

  bool isUnsubscribedOrTerminated() const {
    return !subscription_ || subscription_->isCancelled();
  }

 protected:
  Subscription* subscription() {
    return subscription_.operator->();
  }

  void unsubscribe() {
    CHECK(subscription_);
    subscription_->cancel();
  }

public:
  template <
      typename Next,
      typename =
          typename std::enable_if<folly::is_invocable<Next, T>::value>::type>
  static std::shared_ptr<Observer<T>> create(Next next);

  template <
      typename Next,
      typename Error,
      typename =
          typename std::enable_if<folly::is_invocable<Next, T>::value>::type,
      typename = typename std::enable_if<
          folly::is_invocable<Error, folly::exception_wrapper>::value>::type>
  static std::shared_ptr<Observer<T>> create(Next next, Error error);

  template <
      typename Next,
      typename Error,
      typename Complete,
      typename =
          typename std::enable_if<folly::is_invocable<Next, T>::value>::type,
      typename = typename std::enable_if<
          folly::is_invocable<Error, folly::exception_wrapper>::value>::type,
      typename =
          typename std::enable_if<folly::is_invocable<Complete>::value>::type>
  static std::shared_ptr<Observer<T>>
  create(Next next, Error error, Complete complete);

  static std::shared_ptr<Observer<T>> create() {
    class NullObserver : public Observer<T> {
     public:
      void onNext(T) {}
    };
    return std::make_shared<NullObserver>();
  }

 private:
  std::shared_ptr<Subscription> subscription_;
};

namespace details {

template <typename T, typename Next>
class Base : public Observer<T> {
 public:
  explicit Base(Next next) : next_(std::move(next)) {}

  void onNext(T value) override {
    next_(std::move(value));
  }

 private:
  Next next_;
};

template <typename T, typename Next, typename Error>
class WithError : public Base<T, Next> {
 public:
  WithError(Next next, Error error)
      : Base<T, Next>(std::move(next)), error_(std::move(error)) {}

  void onError(folly::exception_wrapper error) override {
    error_(std::move(error));
  }

 private:
  Error error_;
};

template <typename T, typename Next, typename Error, typename Complete>
class WithErrorAndComplete : public WithError<T, Next, Error> {
 public:
  WithErrorAndComplete(Next next, Error error, Complete complete)
      : WithError<T, Next, Error>(std::move(next), std::move(error)),
        complete_(std::move(complete)) {}

  void onComplete() override {
    complete_();
  }

 private:
  Complete complete_;
};
} // namespace details

template <typename T>
template <typename Next, typename>
std::shared_ptr<Observer<T>> Observer<T>::create(Next next) {
  return std::make_shared<details::Base<T, Next>>(std::move(next));
}

template <typename T>
template <typename Next, typename Error, typename, typename>
std::shared_ptr<Observer<T>> Observer<T>::create(Next next, Error error) {
  return std::make_shared<details::WithError<T, Next, Error>>(
      std::move(next), std::move(error));
}

template <typename T>
template <
    typename Next,
    typename Error,
    typename Complete,
    typename,
    typename,
    typename>
std::shared_ptr<Observer<T>>
Observer<T>::create(Next next, Error error, Complete complete) {
  return std::make_shared<
      details::WithErrorAndComplete<T, Next, Error, Complete>>(
      std::move(next), std::move(error), std::move(complete));
}

} // namespace observable
} // namespace yarpl
