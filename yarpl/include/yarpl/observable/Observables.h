// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <limits>

#include "yarpl/observable/Observable.h"
#include "yarpl/observable/Subscriptions.h"

#include <folly/functional/Invoke.h>

namespace yarpl {
namespace observable {

class Observables {
 public:
  static std::shared_ptr<Observable<int64_t>> range(int64_t start, int64_t end) {
    auto lambda = [start, end](std::shared_ptr<Observer<int64_t>> observer) {
      for (int64_t i = start; i < end; ++i) {
        observer->onNext(i);
      }
      observer->onComplete();
    };

    return Observable<int64_t>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Observable<T>> just(const T& value) {
    auto lambda = [value](std::shared_ptr<Observer<T>> observer) {
      observer->onNext(value);
      observer->onComplete();
    };

    return Observable<T>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Observable<T>> justN(std::initializer_list<T> list) {
    std::vector<T> vec(list);

    auto lambda = [v = std::move(vec)](std::shared_ptr<Observer<T>> observer) {
      for (auto const& elem : v) {
        observer->onNext(elem);
      }
      observer->onComplete();
    };

    return Observable<T>::create(std::move(lambda));
  }

  // this will generate an observable which can be subscribed to only once
  template <typename T>
  static std::shared_ptr<Observable<T>> justOnce(T value) {
    auto lambda = [ value = std::move(value), used = false ](
        std::shared_ptr<Observer<T>> observer) mutable {
      if (used) {
        observer->onError(
            std::runtime_error("justOnce value was already used"));
        return;
      }

      used = true;
      observer->onNext(std::move(value));
      observer->onComplete();
    };

    return Observable<T>::create(std::move(lambda));
  }

  template <
      typename T,
      typename OnSubscribe,
      typename = typename std::enable_if<
          folly::is_invocable<OnSubscribe, std::shared_ptr<Observer<T>>>::value>::
          type>
  static std::shared_ptr<Observable<T>> create(OnSubscribe function) {
    return make_ref<FromPublisherOperator<T, OnSubscribe>>(std::move(function));
  }

  template <typename T>
  static std::shared_ptr<Observable<T>> empty() {
    auto lambda = [](std::shared_ptr<Observer<T>> observer) {
      observer->onComplete();
    };
    return Observable<T>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Observable<T>> error(folly::exception_wrapper ex) {
    auto lambda = [ex = std::move(ex)](std::shared_ptr<Observer<T>> observer) {
      observer->onError(std::move(ex));
    };
    return Observable<T>::create(std::move(lambda));
  }

  template <typename T, typename ExceptionType>
  static std::shared_ptr<Observable<T>> error(const ExceptionType& ex) {
    auto lambda = [ex = std::move(ex)](std::shared_ptr<Observer<T>> observer) {
      observer->onError(std::move(ex));
    };
    return Observable<T>::create(std::move(lambda));
  }

 private:
  Observables() = delete;
};

} // observable
} // yarpl
