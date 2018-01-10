// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/utils/type_traits.h"

#include "yarpl/single/Single.h"
#include "yarpl/single/SingleSubscriptions.h"

#include <folly/functional/Invoke.h>

namespace yarpl {
namespace single {

class Singles {
 public:
  template <typename T>
  static std::shared_ptr<Single<T>> just(const T& value) {
    auto lambda = [value](std::shared_ptr<SingleObserver<T>> observer) {
      observer->onSubscribe(SingleSubscriptions::empty());
      observer->onSuccess(value);
    };

    return Single<T>::create(std::move(lambda));
  }

  template <
      typename T,
      typename OnSubscribe,
      typename = typename std::enable_if<folly::is_invocable<
          OnSubscribe, std::shared_ptr<SingleObserver<T>>>::value>::type>
  static std::shared_ptr<Single<T>> create(OnSubscribe function) {
    return make_ref<FromPublisherOperator<T, OnSubscribe>>(std::move(function));
  }

  template <typename T>
  static std::shared_ptr<Single<T>> error(folly::exception_wrapper ex) {
    auto lambda = [e = std::move(ex)](std::shared_ptr<SingleObserver<T>> observer) {
      observer->onSubscribe(SingleSubscriptions::empty());
      observer->onError(e);
    };
    return Single<T>::create(std::move(lambda));
  }

  template <typename T, typename ExceptionType>
  static std::shared_ptr<Single<T>> error(const ExceptionType& ex) {
    auto lambda = [ex](std::shared_ptr<SingleObserver<T>> observer) {
      observer->onSubscribe(SingleSubscriptions::empty());
      observer->onError(ex);
    };
    return Single<T>::create(std::move(lambda));
  }

  template <typename T, typename TGenerator>
  static std::shared_ptr<Single<T>> fromGenerator(TGenerator generator) {
    auto lambda = [generator = std::move(generator)](
        std::shared_ptr<SingleObserver<T>> observer) mutable {
      observer->onSubscribe(SingleSubscriptions::empty());
      observer->onSuccess(generator());
    };
    return Single<T>::create(std::move(lambda));
  }

 private:
  Singles() = delete;
};

} // observable
} // yarpl
