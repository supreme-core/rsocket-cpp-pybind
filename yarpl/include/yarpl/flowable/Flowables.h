// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include <glog/logging.h>

#include "yarpl/flowable/Flowable.h"

#include <folly/functional/Invoke.h>

namespace yarpl {
namespace flowable {

class Flowables {
 public:
  /**
   * Emit the sequence of numbers [start, start + count).
   */
  static std::shared_ptr<Flowable<int64_t>> range(int64_t start, int64_t count);

  template <typename T>
  static std::shared_ptr<Flowable<T>> just(const T& value) {
    auto lambda = [value](Subscriber<T>& subscriber, int64_t) {
      // # requested should be > 0.  Ignoring the actual parameter.
      subscriber.onNext(value);
      subscriber.onComplete();
    };

    return Flowable<T>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Flowable<T>> justN(std::initializer_list<T> list) {
    std::vector<T> vec(list);

    auto lambda = [ v = std::move(vec), i = size_t{0} ](
        Subscriber<T>& subscriber, int64_t requested) mutable {
      while (i < v.size() && requested-- > 0) {
        subscriber.onNext(v[i++]);
      }

      if (i == v.size()) {
        subscriber.onComplete();
      }
    };

    return Flowable<T>::create(std::move(lambda));
  }

  // this will generate a flowable which can be subscribed to only once
  template <typename T>
  static std::shared_ptr<Flowable<T>> justOnce(T value) {
    auto lambda = [ value = std::move(value), used = false ](
        Subscriber<T>& subscriber, int64_t) mutable {
      if (used) {
        subscriber.onError(
            std::runtime_error("justOnce value was already used"));
        return;
      }

      used = true;
      // # requested should be > 0.  Ignoring the actual parameter.
      subscriber.onNext(std::move(value));
      subscriber.onComplete();
    };

    return Flowable<T>::create(std::move(lambda));
  }

  template <
      typename T,
      typename OnSubscribe,
      typename = typename std::enable_if<folly::is_invocable<
          OnSubscribe, std::shared_ptr<Subscriber<T>>>::value>::type>
  static std::shared_ptr<Flowable<T>> fromPublisher(OnSubscribe function) {
    return make_ref<FromPublisherOperator<T, OnSubscribe>>(std::move(function));
  }

  template <typename T>
  static std::shared_ptr<Flowable<T>> empty() {
    auto lambda = [](Subscriber<T>& subscriber, int64_t) {
      subscriber.onComplete();
    };
    return Flowable<T>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Flowable<T>> never() {
    auto lambda = [](details::TrackingSubscriber<T>& subscriber, int64_t) {
      subscriber.setCompleted();
    };
    return Flowable<T>::create(std::move(lambda));
  }

  template <typename T>
  static std::shared_ptr<Flowable<T>> error(folly::exception_wrapper ex) {
    auto lambda = [ex = std::move(ex)](
        Subscriber<T>& subscriber, int64_t) {
      subscriber.onError(std::move(ex));
    };
    return Flowable<T>::create(std::move(lambda));
  }

  template <typename T, typename ExceptionType>
  static std::shared_ptr<Flowable<T>> error(const ExceptionType& ex) {
    auto lambda = [ex = std::move(ex)](
        Subscriber<T>& subscriber, int64_t) {
      subscriber.onError(std::move(ex));
    };
    return Flowable<T>::create(std::move(lambda));
  }

  template <typename T, typename TGenerator>
  static std::shared_ptr<Flowable<T>> fromGenerator(TGenerator generator) {
    auto lambda = [generator = std::move(generator)](
        Subscriber<T>& subscriber, int64_t requested) {
      try {
        while (requested-- > 0) {
          subscriber.onNext(generator());
        }
      } catch (const std::exception& ex) {
        subscriber.onError(
            folly::exception_wrapper(std::current_exception(), ex));
      } catch (...) {
        subscriber.onError(std::runtime_error("unknown error"));
      }
    };
    return Flowable<T>::create(std::move(lambda));
  }

 private:
  Flowables() = delete;
};

} // flowable
} // yarpl
