// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Refcounted.h"
#include "yarpl/single/SingleSubscription.h"

#include <folly/ExceptionWrapper.h>

#include <glog/logging.h>

namespace yarpl {
namespace single {

template <typename T>
class SingleObserver : public yarpl::enable_get_ref {
 public:
  virtual ~SingleObserver() = default;
  virtual void onSubscribe(std::shared_ptr<SingleSubscription>) = 0;
  virtual void onSuccess(T) = 0;
  virtual void onError(folly::exception_wrapper) = 0;

  template <typename Success>
  static std::shared_ptr<SingleObserver<T>> create(Success success);

  template <typename Success, typename Error>
  static std::shared_ptr<SingleObserver<T>> create(
      Success success,
      Error error);
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
class SingleObserverBase<void> {
 public:
  virtual ~SingleObserverBase() = default;

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

template <typename T, typename Success, typename Error>
class SimpleSingleObserver : public SingleObserver<T> {
 public:
  SimpleSingleObserver(Success success, Error error)
      : success_(std::move(success)), error_(std::move(error)) {}

  void onSubscribe(std::shared_ptr<SingleSubscription>) {
    // throw away the subscription
  }

  void onSuccess(T value) override {
    success_(std::move(value));
  }

  void onError(folly::exception_wrapper ew) {
    error_(std::move(ew));
  }

  Success success_;
  Error error_;
};

template <typename T>
template <typename Success>
std::shared_ptr<SingleObserver<T>> SingleObserver<T>::create(Success success) {
  static_assert(
      folly::is_invocable<Success, T>::value,
      "Input `success` should be invocable with a parameter of `T`.");
  return std::make_shared<SimpleSingleObserver<
      T,
      Success,
      folly::Function<void(folly::exception_wrapper)>>>(
      std::move(success), [](folly::exception_wrapper) {});
}

template <typename T>
template <typename Success, typename Error>
std::shared_ptr<SingleObserver<T>> SingleObserver<T>::create(
    Success success,
    Error error) {
  static_assert(
      folly::is_invocable<Success, T>::value,
      "Input `success` should be invocable with a parameter of `T`.");
  static_assert(
      folly::is_invocable<Error, folly::exception_wrapper>::value,
      "Input `error` should be invocable with a parameter of "
      "`folly::exception_wrapper`.");

  return std::make_shared<SimpleSingleObserver<T, Success, Error>>(
      std::move(success), std::move(error));
}

} // namespace single
} // namespace yarpl
