#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <utility>

#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include <folly/ExceptionWrapper.h>
#include <folly/Poly.h>
#include <folly/poly/Regular.h>

namespace yarpl {
namespace erased {
namespace flowable {

#define YARPL_EAGERLY_ERASE_TYPES 0

namespace detail {
using PolyMoveOnly = folly::PolyExtends<folly::poly::IMoveOnly>;

/**
 * The Subscription interface for Poly
 */
struct ISubscription : PolyMoveOnly {
  // Requesting methods
  template <class Base>
  struct Interface : Base {
    void request(int64_t n) {
      folly::poly_call<0>(*this, n);
    }
    void cancel() {
      folly::poly_call<1>(*this);
    }
  };
  template <class T>
  using Members = FOLLY_POLY_MEMBERS(&T::request, &T::cancel);
};

/**
 * The Subscriber interface for Poly
 */
template <typename U>
struct ISubscriber : PolyMoveOnly {
  template <class Base>
  struct Interface : Base {
    // Signaling methods
    void onSubscribe(folly::Poly<ISubscription&> subscription) {
      folly::poly_call<0>(*this, subscription);
    }
    void onNext(U u) {
      folly::poly_call<1>(*this, std::move(u));
    }
    void onError(folly::exception_wrapper ew) {
      folly::poly_call<2>(*this, std::move(ew));
    }
    void onComplete() {
      folly::poly_call<3>(*this);
    }
  };
  template <class T>
  using Members = FOLLY_POLY_MEMBERS(
      &T::onSubscribe,
      &T::onNext,
      &T::onError,
      &T::onComplete);
};

/**
 * The Flowable interface for Poly
 */
template <typename U>
struct IFlowable : PolyMoveOnly {
  template <class Base>
  struct Interface : Base {
    using Type = U;

    folly::Poly<ISubscription> subscribe(
        folly::Poly<ISubscriber<U>> subscriber) const {
      return folly::poly_call<0>(*this, std::move(subscriber));
    }
    // more....
  };
  template <class T>
  using Members = FOLLY_POLY_MEMBERS(&T::subscribe);
};

template <typename>
struct MemberFuncArg;

template <typename Type, typename ArgType>
struct MemberFuncArg<void (Type::*)(ArgType)> {
  using SubType = Type;
  using arg1_type = ArgType;
};

template <typename Subscriber>
using GetSubscriberType =
    typename MemberFuncArg<decltype(&Subscriber::onNext)>::arg1_type;

} // namespace detail

// type-erasing wrappers
using AnySubscription = folly::Poly<detail::ISubscription>;
template <class T>
using AnySubscriber = folly::Poly<detail::ISubscriber<T>>;
template <class T>
using AnyFlowable = folly::Poly<detail::IFlowable<T>>;

// Non-owning reference types
using AnySubscriptionRef = folly::Poly<detail::ISubscription&>;
template <class T>
using AnySubscriberRef = folly::Poly<detail::ISubscriber<T>&>;

// `nop` subscription
struct EmptySubscription {
  void request(int64_t) {}
  void cancel() {}
};

// Tag for wrapping generating flowable lambdas in
template <typename T, typename Impl>
struct Flowable {
  Flowable(Impl&& fi) : flowableImpl(std::move(fi)) {}
  Flowable(Flowable&& other) = default;
  Flowable(Flowable const&) = delete;

  using Type = T;

  template <
      typename Subscriber,
      typename = std::enable_if_t<std::is_same<
          typename detail::GetSubscriberType<Subscriber>,
          T>::value>>
  auto subscribe(Subscriber s) const {
    return flowableImpl(std::move(s));
  }

  auto subscribe(AnySubscriber<T> rhs) const {
    return flowableImpl(std::move(rhs));
  }

 private:
  Impl flowableImpl;
};

// TODO: move isFlowable into a traits utility namespace
namespace detail {
template <typename>
struct isFlowable : std::false_type {};

template <typename T, typename Impl>
struct isFlowable<Flowable<T, Impl>> : std::true_type {
  using Type = T;
};

template <typename T>
struct isFlowable<AnyFlowable<T>> : std::true_type {
  using Type = T;
};

template <typename T>
struct assertIsFlowable {
  constexpr static bool value = detail::isFlowable<T>::value;
  static_assert(value, "T is not a Flowable type!");
};

// Wrappers for maybe erasing the given type - may be helpful for simplifying
// debugging if types start to get huge in deeply nested pipelines
#if YARPL_EAGERLY_ERASE_TYPES
template <
    typename SubscriberT,
    typename T = typename detail::GetSubscriberType<SubscriberT>>
AnySubscriber<T> maybeEraseSubscriber(SubscriberT s) {
  return AnySubscriber<T>(std::move(s));
}

template <typename SubscriptionT>
AnySubscription maybeEraseSubscription(SubscriptionT s) {
  return AnySubscription(std::move(s));
}

template <
    typename FlowableT,
    typename T = typename detail::isFlowable<FlowableT>::Type>
AnyFlowable<T> maybeEraseFlowable(FlowableT f) {
  return AnyFlowable<T>(std::move(f));
}
#else
template <
    typename SubscriberT,
    typename T = typename detail::GetSubscriberType<SubscriberT>>
SubscriberT maybeEraseSubscriber(SubscriberT s) {
  return s;
}

template <typename SubscriptionT>
SubscriptionT maybeEraseSubscription(SubscriptionT s) {
  return s;
}

template <
    typename FlowableT,
    typename T = typename detail::isFlowable<FlowableT>::Type>
FlowableT maybeEraseFlowable(FlowableT f) {
  return f;
}
#endif
} // namespace detail

// Tag for wrapping intermediary flowable operators in
template <typename Impl>
struct FlowableOperator {
  FlowableOperator(Impl&& fi) : operatorImpl(std::move(fi)) {}
  FlowableOperator(FlowableOperator const&) = delete;
  FlowableOperator(FlowableOperator&&) = default;
  Impl operatorImpl;
};

template <typename T, typename Impl>
auto makeFlowable(Impl impl) {
  return Flowable<T, Impl>(std::move(impl));
}

template <typename Impl>
auto makeOperator(Impl impl) {
  return FlowableOperator<Impl>(std::move(impl));
}

} // namespace flowable
} // namespace erased
} // namespace yarpl
