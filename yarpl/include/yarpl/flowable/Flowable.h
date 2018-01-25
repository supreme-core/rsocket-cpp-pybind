// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <glog/logging.h>

#include "yarpl/Refcounted.h"
#include "yarpl/flowable/Subscriber.h"
#include "yarpl/flowable/Subscribers.h"
#include "yarpl/utils/credits.h"
#include "yarpl/utils/type_traits.h"

#include <folly/Executor.h>
#include <folly/functional/Invoke.h>

namespace yarpl {
namespace flowable {

template <typename T>
class Flowable;

namespace details {

template <typename T>
struct IsFlowable : std::false_type {};

template <typename R>
struct IsFlowable<std::shared_ptr<Flowable<R>>> : std::true_type {
  using ElemType = R;
};

template <typename T>
class TrackingSubscriber;

} // namespace details

template <typename T>
class Flowable : public yarpl::enable_get_ref {
 public:
   virtual ~Flowable() = default;

  virtual void subscribe(std::shared_ptr<Subscriber<T>>) = 0;

  /**
   * Subscribe overload that accepts lambdas.
   */
  template <
      typename Next,
      typename =
          typename std::enable_if<folly::is_invocable<Next, T>::value>::type>
  void subscribe(Next next, int64_t batch = credits::kNoFlowControl) {
    subscribe(Subscribers::create<T>(std::move(next), batch));
  }

  /**
   * Subscribe overload that accepts lambdas.
   *
   * Takes an optional batch size for request_n. Default is no flow control.
   */
  template <
      typename Next,
      typename Error,
      typename = typename std::enable_if<
          folly::is_invocable<Next, T>::value &&
          folly::is_invocable<Error, folly::exception_wrapper>::value>::type>
  void
  subscribe(Next next, Error error, int64_t batch = credits::kNoFlowControl) {
    subscribe(Subscribers::create<T>(std::move(next), std::move(error), batch));
  }

  /**
   * Subscribe overload that accepts lambdas.
   *
   * Takes an optional batch size for request_n. Default is no flow control.
   */
  template <
      typename Next,
      typename Error,
      typename Complete,
      typename = typename std::enable_if<
          folly::is_invocable<Next, T>::value &&
          folly::is_invocable<Error, folly::exception_wrapper>::value &&
          folly::is_invocable<Complete>::value>::type>
  void subscribe(
      Next next,
      Error error,
      Complete complete,
      int64_t batch = credits::kNoFlowControl) {
    subscribe(Subscribers::create<T>(
        std::move(next), std::move(error), std::move(complete), batch));
  }

  template <
      typename Function,
      typename R = typename std::result_of<Function(T)>::type>
  std::shared_ptr<Flowable<R>> map(Function function);

  template <
      typename Function,
      typename R = typename details::IsFlowable<
          typename std::result_of<Function(T)>::type>::ElemType>
  std::shared_ptr<Flowable<R>> flatMap(Function func);

  template <typename Function>
  std::shared_ptr<Flowable<T>> filter(Function function);

  template <
      typename Function,
      typename R = typename std::result_of<Function(T, T)>::type>
  std::shared_ptr<Flowable<R>> reduce(Function function);

  std::shared_ptr<Flowable<T>> take(int64_t);

  std::shared_ptr<Flowable<T>> skip(int64_t);

  std::shared_ptr<Flowable<T>> ignoreElements();

  std::shared_ptr<Flowable<T>> subscribeOn(folly::Executor&);

  std::shared_ptr<Flowable<T>> observeOn(folly::Executor&);

  template <typename Q>
  using enableWrapRef =
      typename std::enable_if<details::IsFlowable<Q>::value, Q>::type;

  template <typename Q = T>
  enableWrapRef<Q> merge() {
    return this->flatMap([](auto f) { return std::move(f); });
  }

  template <
      typename Emitter,
      typename = typename std::enable_if<folly::is_invocable_r<
          void,
          Emitter, details::TrackingSubscriber<T>&, int64_t
          >::value>::type>
  static std::shared_ptr<Flowable<T>> create(Emitter emitter);
};

} // flowable
} // yarpl

#include "yarpl/flowable/EmitterFlowable.h"
#include "yarpl/flowable/FlowableOperator.h"

namespace yarpl {
namespace flowable {

template <typename T>
template <typename Emitter, typename>
std::shared_ptr<Flowable<T>> Flowable<T>::create(Emitter emitter) {
  return std::make_shared<details::EmitterWrapper<T, Emitter>>(std::move(emitter));
}

template <typename T>
template <typename Function, typename R>
std::shared_ptr<Flowable<R>> Flowable<T>::map(Function function) {
  return std::make_shared<MapOperator<T, R, Function>>(
      this->ref_from_this(this), std::move(function));
}

template <typename T>
template <typename Function>
std::shared_ptr<Flowable<T>> Flowable<T>::filter(Function function) {
  return std::make_shared<FilterOperator<T, Function>>(
      this->ref_from_this(this), std::move(function));
}

template <typename T>
template <typename Function, typename R>
std::shared_ptr<Flowable<R>> Flowable<T>::reduce(Function function) {
  return std::make_shared<ReduceOperator<T, R, Function>>(
      this->ref_from_this(this), std::move(function));
}

template <typename T>
std::shared_ptr<Flowable<T>> Flowable<T>::take(int64_t limit) {
  return std::make_shared<TakeOperator<T>>(this->ref_from_this(this), limit);
}

template <typename T>
std::shared_ptr<Flowable<T>> Flowable<T>::skip(int64_t offset) {
  return std::make_shared<SkipOperator<T>>(this->ref_from_this(this), offset);
}

template <typename T>
std::shared_ptr<Flowable<T>> Flowable<T>::ignoreElements() {
  return std::make_shared<IgnoreElementsOperator<T>>(this->ref_from_this(this));
}

template <typename T>
std::shared_ptr<Flowable<T>> Flowable<T>::subscribeOn(folly::Executor& executor) {
  return std::make_shared<SubscribeOnOperator<T>>(this->ref_from_this(this), executor);
}

template <typename T>
std::shared_ptr<Flowable<T>> Flowable<T>::observeOn(folly::Executor& executor) {
  return std::make_shared<yarpl::flowable::detail::ObserveOnOperator<T>>(
      this->ref_from_this(this), executor);
}

template <typename T>
template <typename Function, typename R>
std::shared_ptr<Flowable<R>> Flowable<T>::flatMap(Function function) {
  return std::make_shared<FlatMapOperator<T, R>>(
      this->ref_from_this(this), std::move(function));
}

} // flowable
} // yarpl
