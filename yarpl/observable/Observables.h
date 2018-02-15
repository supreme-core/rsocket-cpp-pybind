// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/functional/Invoke.h>
#include <limits>
#include "yarpl/observable/Observable.h"
#include "yarpl/observable/Subscriptions.h"

namespace yarpl {
namespace observable {

template<>
class Observable<void> {
 public:
  /**
   * Emit the sequence of numbers [start, start + count).
   */
  static std::shared_ptr<Observable<int64_t>> range(
      int64_t start,
      int64_t count);

  template <typename T>
  static std::shared_ptr<Observable<T>> just(T value) {
    return Observable<T>::just(std::move(value));
  }

  template <typename T>
  static std::shared_ptr<Observable<T>> justN(std::initializer_list<T> list) {
    return Observable<T>::justN(std::move(list));
  }

  // this will generate an observable which can be subscribed to only once
  template <typename T>
  static std::shared_ptr<Observable<T>> justOnce(T value) {
    return Observable<T>::justOnce(std::move(value));
  }

 private:
  Observable() = delete;
};

} // observable
} // yarpl
