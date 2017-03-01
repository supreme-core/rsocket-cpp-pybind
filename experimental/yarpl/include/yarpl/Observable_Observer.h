// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <yarpl/Observable_Subscription.h>

namespace yarpl {
namespace observable {

/**
 *
 */
template <typename T> class Observer {
public:
 virtual ~Observer() = default;

 static std::unique_ptr<Observer<T>> create(
     std::function<void(const T&)> onNext);

 static std::unique_ptr<Observer<T>> create(
     std::function<void(const T&)> onNext,
     std::function<void(const std::exception&)> onError);

 static std::unique_ptr<Observer<T>> create(
     std::function<void(const T&)> onNext,
     std::function<void(const std::exception&)> onError,
     std::function<void()> onComplete);

 virtual void onNext(const T& value) = 0;
 virtual void onError(const std::exception& e) = 0;
 virtual void onComplete() = 0;
 virtual void onSubscribe(std::unique_ptr<Subscription>) = 0;
};

/* ****************************************** */
/* implementation here because of templates */
/* https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl */
/* ****************************************** */
// TODO: move the constants to the cpp file
static const std::function<void(const std::exception&)> kDEFAULT_ERROR =
    [](const std::exception&) {};
static const std::function<void()> kDEFAULT_COMPLETE = []() {};

template <typename T> class ObserverImpl : public Observer<T> {
 public:
  explicit ObserverImpl(std::function<void(const T&)> n)
      : next_(std::move(n)),
        error_(kDEFAULT_ERROR),
        complete_(kDEFAULT_COMPLETE) {}
  ObserverImpl(
      std::function<void(const T&)> n,
      std::function<void(const std::exception&)> e)
      : next_(std::move(n)),
        error_(std::move(e)),
        complete_(kDEFAULT_COMPLETE) {}
  ObserverImpl(
      std::function<void(const T&)> n,
      std::function<void(const std::exception&)> e,
      std::function<void()> c)
      : next_(std::move(n)), error_(std::move(e)), complete_(std::move(c)) {}

  void onNext(const T& value) {
    next_(value);
  }

  void onError(const std::exception& e) {
    error_(e);
  }

  void onComplete() {
    complete_();
  }

  void onSubscribe(std::unique_ptr<Subscription> s) {
    // do nothing with onSubscribe when lambdas used
    // cancellation is not supported in that case
  }

 private:
  std::function<void(const T&)> next_;
  std::function<void(const std::exception&)> error_;
  std::function<void()> complete_;
};

template <typename T>
std::unique_ptr<Observer<T>>
Observer<T>::create(std::function<void(const T &)> onNext) {
  return std::unique_ptr<Observer<T>>(new ObserverImpl<T>(onNext));
}

template <typename T>
std::unique_ptr<Observer<T>>
Observer<T>::create(std::function<void(const T &)> onNext,
                    std::function<void(const std::exception &)> onError) {
  return std::unique_ptr<Observer<T>>(new ObserverImpl<T>(onNext, onError));
}

template <typename T>
std::unique_ptr<Observer<T>>
Observer<T>::create(std::function<void(const T &)> onNext,
                    std::function<void(const std::exception &)> onError,
                    std::function<void()> onComplete) {
  return std::unique_ptr<Observer<T>>(
      new ObserverImpl<T>(onNext, onError, onComplete));
}

} // observable namespace
} // yarpl namespace
