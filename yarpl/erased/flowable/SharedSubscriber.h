#pragma once

#include "yarpl/erased/flowable/Flowable.h"

namespace yarpl {
namespace erased {
namespace flowable {
// Convert a move-only type subscriber value into one which can be copied
// (and is held as a shared value under the hood)
template <typename SubscriberType>
class SharedSubscriber {
  using T = typename detail::GetSubscriberType<SubscriberType>;

 public:
  explicit SharedSubscriber(SubscriberType&& sub)
      : sharedSubscriber_(std::make_shared<SubscriberType>(std::move(sub))) {}

  SharedSubscriber() : sharedSubscriber_(nullptr) {}
  SharedSubscriber(std::nullptr_t) : sharedSubscriber_(nullptr) {}

  SharedSubscriber(SharedSubscriber const&) = default;
  SharedSubscriber(SharedSubscriber&& other)
      : sharedSubscriber_(std::move(other.sharedSubscriber_)) {}

  explicit SharedSubscriber(std::shared_ptr<SubscriberType>&& sub)
      : sharedSubscriber_(std::move(sub)) {}

  explicit SharedSubscriber(std::shared_ptr<SubscriberType> const& sub)
      : sharedSubscriber_(sub) {}

  void onSubscribe(AnySubscriptionRef subscriber) {
#ifndef NDEBUG
    CHECK(!wasSubscribed_);
    CHECK(!wasTermianted_);
    wasSubscribed_ = true;
#endif
    if (sharedSubscriber_) {
      sharedSubscriber_->onSubscribe(subscriber);
    }
  }

  void onNext(T elem) {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
#endif
    if (sharedSubscriber_) {
      sharedSubscriber_->onNext(std::move(elem));
    }
  }

  void onComplete() {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
    wasTermianted_ = true;
#endif

    if (auto s = sharedSubscriber_) {
      s->onComplete();
    }
  }

  void onError(folly::exception_wrapper err) {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
    wasTermianted_ = true;
#endif

    if (auto s = sharedSubscriber_) {
      s->onError(std::move(err));
    }
  }

  SubscriberType& underlying() const {
    CHECK(sharedSubscriber_);
    return *sharedSubscriber_;
  }
  SubscriberType* operator->() const {
    return &underlying();
  }

  void clear() {
    sharedSubscriber_.reset();
  }

  void cleared() const {
    return !!sharedSubscriber_;
  }

 private:
#ifndef NDEBUG
  bool wasSubscribed_{false};
  bool wasTermianted_{false};
#endif
  std::shared_ptr<SubscriberType> sharedSubscriber_;
};

template <typename Subscriber>
auto makeSharedSubscriber(Subscriber&& subscriber) {
  return SharedSubscriber<Subscriber>(std::move(subscriber));
}

} // namespace flowable
} // namespace erased
} // namespace yarpl
