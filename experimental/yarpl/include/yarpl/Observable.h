// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <functional>
#include <memory>
#include "yarpl/Observable_Observer.h"

namespace yarpl {
namespace observable {

template <typename T>
class Observable {
 public:
  explicit Observable(
      std::function<void(std::unique_ptr<Observer<T>>)> onSubscribe);

  static std::unique_ptr<Observable<T>> create(
      std::function<void(std::unique_ptr<Observer<T>>)> onSubscribe);

  void subscribe(std::unique_ptr<Observer<T>>);

 private:
  std::function<void(std::unique_ptr<Observer<T>>)> onSubscribe_;
};

/* ****************************************** */
/* implementation here because of templates */
/* https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl */
/* ****************************************** */

template <typename T>
Observable<T>::Observable(std::function<void(std::unique_ptr<Observer<T>>)> os)
    : onSubscribe_(std::move(os)) {}

template <typename T>
std::unique_ptr<Observable<T>> Observable<T>::create(
    std::function<void(std::unique_ptr<Observer<T>>)> onSubscribe) {
  return std::make_unique<Observable>(std::move(onSubscribe));
}

template <typename T>
void Observable<T>::subscribe(std::unique_ptr<Observer<T>> o) {
  // when subscribed to, invoke the `onSubscribe` function
  onSubscribe_(std::move(o));
}

} // observable namespace
} // yarpl namespace
