// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/functional/Invoke.h>
#include <glog/logging.h>
#include <algorithm>
#include <limits>
#include <vector>
#include "yarpl/flowable/Flowable.h"

namespace yarpl {
namespace flowable {

template<>
class Flowable<void> {
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

 private:
  Flowable() = delete;
};

} // flowable
} // yarpl
