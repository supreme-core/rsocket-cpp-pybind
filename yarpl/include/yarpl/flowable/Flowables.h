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
  static std::shared_ptr<Flowable<T>> just(T value) {
    return Flowable<T>::just(std::move(value));
  }

  template <typename T>
  static std::shared_ptr<Flowable<T>> justN(std::initializer_list<T> list) {
    return Flowable<T>::justN(std::move(list));
  }

  // this will generate a flowable which can be subscribed to only once
  template <typename T>
  static std::shared_ptr<Flowable<T>> justOnce(T value) {
    return Flowable<T>::justOnce(std::move(value));
  }

 private:
  Flowable() = delete;
};

} // flowable
} // yarpl
