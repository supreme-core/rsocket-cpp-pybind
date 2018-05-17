// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Refcounted.h"

namespace yarpl {
namespace flowable {

class Subscription {
 public:
  virtual ~Subscription() = default;

  virtual void request(int64_t n) = 0;
  virtual void cancel() = 0;

  static std::shared_ptr<Subscription> create();

  template <typename CancelFunc>
  static std::shared_ptr<Subscription> create(CancelFunc&& onCancel);

  template <typename CancelFunc, typename RequestFunc>
  static std::shared_ptr<Subscription> create(
      CancelFunc&& onCancel,
      RequestFunc&& onRequest);
};

namespace detail {

template <typename CancelFunc, typename RequestFunc>
class CallbackSubscription : public Subscription {
  static_assert(
      std::is_same<std::decay_t<CancelFunc>, CancelFunc>::value,
      "undecayed");
  static_assert(
      std::is_same<std::decay_t<RequestFunc>, RequestFunc>::value,
      "undecayed");

 public:
  template <typename FCancel, typename FRequest>
  CallbackSubscription(FCancel&& onCancel, FRequest&& onRequest)
      : onCancel_(std::forward<FCancel>(onCancel)),
        onRequest_(std::forward<FRequest>(onRequest)) {}

  void request(int64_t n) override {
    onRequest_(n);
  }
  void cancel() override {
    onCancel_();
  }

 private:
  CancelFunc onCancel_;
  RequestFunc onRequest_;
};
} // namespace detail

template <typename CancelFunc, typename RequestFunc>
std::shared_ptr<Subscription> Subscription::create(
    CancelFunc&& onCancel,
    RequestFunc&& onRequest) {
  return std::make_shared<detail::CallbackSubscription<
      std::decay_t<CancelFunc>,
      std::decay_t<RequestFunc>>>(
      std::forward<CancelFunc>(onCancel), std::forward<RequestFunc>(onRequest));
}

template <typename CancelFunc>
std::shared_ptr<Subscription> Subscription::create(CancelFunc&& onCancel) {
  return Subscription::create(
      std::forward<CancelFunc>(onCancel), [](int64_t) {});
}

} // namespace flowable
} // namespace yarpl
