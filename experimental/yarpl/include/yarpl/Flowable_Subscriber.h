// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Flowable_Subscription.h"

namespace yarpl {
namespace flowable {

/**
 *
 */
template <typename T>
class Subscriber {
 public:
  virtual ~Subscriber() = default;

  static std::unique_ptr<Subscriber<T>> create(
      std::function<void(const T&)> onNext);

  static std::unique_ptr<Subscriber<T>> create(
      std::function<void(const T&)> onNext,
      std::function<void(const std::exception&)> onError);

  static std::unique_ptr<Subscriber<T>> create(
      std::function<void(const T&)> onNext,
      std::function<void(const std::exception&)> onError,
      std::function<void()> onComplete);

  // TODO: make the folowing pure virtual
  virtual void onNext(const T& value) {}
  virtual void onError(const std::exception& e) = 0;
  virtual void onComplete() = 0;
  virtual void onSubscribe(std::unique_ptr<Subscription>) = 0;
};

/* ****************************************** */
/* implementation here because of templates */
/* https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl */
/* ****************************************** */
// TODO: move the constants to the cpp file
static const std::function<void(const std::exception&)> kDefaultError =
    [](const std::exception&) {};
static const std::function<void()> kDefaultComplete = []() {};

template <typename T>
class ObserverImpl : public Subscriber<T> {
 public:
  explicit ObserverImpl(std::function<void(const T&)> n)
      : next_(std::move(n)),
        error_(kDefaultError),
        complete_(kDefaultComplete) {}
  ObserverImpl(
      std::function<void(const T&)> n,
      std::function<void(const std::exception&)> e)
      : next_(std::move(n)),
        error_(std::move(e)),
        complete_(kDefaultComplete){};
  ObserverImpl(
      std::function<void(const T&)> n,
      std::function<void(const std::exception&)> e,
      std::function<void()> c)
      : next_(std::move(n)), error_(std::move(e)), complete_(std::move(c)) {}

  void onNext(const T& value) override {
    next_(value);
  }

  void onError(const std::exception& e) override {
    error_(e);
  }

  void onComplete() override {
    complete_();
  }

  void onSubscribe(std::unique_ptr<Subscription> s) override {
    // default to requesting max() (infinite)
    // which means this Subscriber is synchronous and needs no flow control
    s->request(std::numeric_limits<uint64_t>::max());
  }

 private:
  std::function<void(const T&)> next_;
  std::function<void(const std::exception&)> error_;
  std::function<void()> complete_;
};

template <typename T>
std::unique_ptr<Subscriber<T>> Subscriber<T>::create(
    std::function<void(const T&)> onNext) {
  return std::make_unique<ObserverImpl<T>>(std::move(onNext));
}

template <typename T>
std::unique_ptr<Subscriber<T>> Subscriber<T>::create(
    std::function<void(const T&)> onNext,
    std::function<void(const std::exception&)> onError) {
  return std::make_unique<ObserverImpl<T>>(
      std::move(onNext), std::move(onError));
}

template <typename T>
std::unique_ptr<Subscriber<T>> Subscriber<T>::create(
    std::function<void(const T&)> onNext,
    std::function<void(const std::exception&)> onError,
    std::function<void()> onComplete) {
  return std::make_unique<ObserverImpl<T>>(
      std::move(onNext), std::move(onError), std::move(onComplete));
}

} // observable namespace
} // yarpl namespace
