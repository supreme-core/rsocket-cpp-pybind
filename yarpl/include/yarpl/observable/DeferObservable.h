// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/observable/Observable.h"

namespace yarpl {
namespace observable {
namespace details {

template <typename T, typename ObservableFactory>
class DeferObservable : public Observable<T> {
 public:
  DeferObservable(ObservableFactory factory) : factory_(std::move(factory)) {}

  virtual std::shared_ptr<Subscription> subscribe(
      std::shared_ptr<Observer<T>> observer) {
    std::shared_ptr<Observable<T>> observable;
    try {
      observable = factory_();
    } catch (const std::exception& ex) {
      observable = Observable<T>::error(ex, std::current_exception());
    }
    return observable->subscribe(std::move(observer));
  }

 private:
  ObservableFactory factory_;
};

} // namespace details
} // namespace observable
} // namespace yarpl
