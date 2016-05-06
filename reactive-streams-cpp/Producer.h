// Copyright 2004-present Facebook. All Rights Reserved.


#pragma once

#include <exception>

namespace lithium {
namespace reactivestreams {

template <typename T, typename E>
class Subscriber;

/// Provides a potentially infinite sequence of elements of type T.
///
/// Rules:
/// TODO(stupaq):
///   * copy from specification for JVM,
///
/// Life cycle considerations:
/// 1. The Producer is not owned by Subscriber or Subscription.
/// 2. The Producer can be a temporary object, as it is only used to capture the
///   indirection in creation of the Subscription instance. Producer's lifetime
///   does not need to extend beyond a lifetime of any of the Subscribers.
template <typename T, typename E = std::exception_ptr>
class Producer {
 public:
  virtual ~Producer() = default;

  /// Establishes an abstract subscription between Subscriber and Producer, by
  /// providing the former with an instance of Subscription.
  ///
  /// Must call Subscriber::onSubscribe synchronously to provide a valid
  /// Subscription.
  ///
  /// Life cycle considerations:
  /// 1. No ownership of the Subscriber is assumed by the Producer.
  /// 2. The Subsciber pointer MUST remain valid until the Producer calls
  ///   Subscriber::{onComplete,onError}. See "unsubscribe handshake" for more
  ///   details.
  virtual void subscribe(Subscriber<T, E>& subscriber) = 0;
};
}
}
