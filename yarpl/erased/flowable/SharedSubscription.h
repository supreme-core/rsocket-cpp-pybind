#pragma once

#include "yarpl/erased/flowable/Flowable.h"

namespace yarpl {
namespace erased {
namespace flowable {
// Convert a move-only type subscription value into one which can be copied
// (and is held as a shared value under the hood)
template <typename SubscriptionType>
class SharedSubscription {
 public:
  explicit SharedSubscription(SubscriptionType&& sub)
      : sharedSubscription_(
            std::make_shared<SubscriptionType>(std::move(sub))) {}

  explicit SharedSubscription(std::shared_ptr<SubscriptionType>&& sub)
      : sharedSubscription_(std::move(sub)) {}

  explicit SharedSubscription(std::shared_ptr<SubscriptionType> const& sub)
      : sharedSubscription_(sub) {}

  SharedSubscription() : sharedSubscription_(nullptr) {}
  SharedSubscription(std::nullptr_t) : sharedSubscription_(nullptr) {}

  SharedSubscription(SharedSubscription const&) = default;
  SharedSubscription(SharedSubscription&& other)
      : sharedSubscription_(std::move(other.sharedSubscription_)) {}

  SharedSubscription& operator=(SharedSubscription&& other) {
    sharedSubscription_ = std::move(other.sharedSubscription_);
    return *this;
  }

  void cancel() {
    if (sharedSubscription_) {
      sharedSubscription_->cancel();
    }
  }
  void request(int64_t n) {
    if (sharedSubscription_) {
      sharedSubscription_->request(n);
    }
  }

  SubscriptionType& underlying() const {
    CHECK(sharedSubscription_);
    return *sharedSubscription_;
  }

  bool cleared() const {
    return !!sharedSubscription_;
  }

  void clear() {
    sharedSubscription_.reset();
  }

 private:
  std::shared_ptr<SubscriptionType> sharedSubscription_;
};

template <typename Subscription>
auto makeSharedSubscription(Subscription&& subscription) {
  return SharedSubscription<Subscription>(std::move(subscription));
}

} // namespace flowable
} // namespace erased
} // namespace yarpl
