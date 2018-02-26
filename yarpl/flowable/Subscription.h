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

  static std::shared_ptr<Subscription> empty(); // TODO: rename to create

  template <typename CancelFunc>
  static std::shared_ptr<Subscription> create(CancelFunc onCancel);

  template <typename CancelFunc, typename RequestFunc>
  static std::shared_ptr<Subscription> create(
      CancelFunc onCancel,
      RequestFunc onRequest);
};

namespace detail {

template <typename CancelFunc, typename RequestFunc>
class CallbackSubscription : public Subscription {
 public:
  CallbackSubscription(CancelFunc onCancel, RequestFunc onRequest)
      : onCancel_(std::move(onCancel)), onRequest_(std::move(onRequest)) {}

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
    CancelFunc onCancel,
    RequestFunc onRequest) {
  return std::make_shared<detail::CallbackSubscription<CancelFunc, RequestFunc>>(
      std::move(onCancel), std::move(onRequest));
}

template <typename CancelFunc>
std::shared_ptr<Subscription> Subscription::create(CancelFunc onCancel) {
  return Subscription::create(std::move(onCancel), [](int64_t) {});
}

} // namespace flowable
} // namespace yarpl
