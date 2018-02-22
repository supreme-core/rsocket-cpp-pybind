#pragma once
#include "yarpl/erased/flowable/Flowable.h"

namespace yarpl {
namespace erased {
namespace flowable {
// Convert a move-only type subscriber value into one which can be copied
// (and is held as a shared value under the hood)
template <typename CompoundType>
class SharedCompound {
  using T = typename detail::MemberFuncArg<decltype(
      &CompoundType::onNext)>::arg1_type;

 public:
  explicit SharedCompound(CompoundType&& sub)
      : sharedCompound_(std::make_shared<CompoundType>(std::move(sub))) {}

  explicit SharedCompound() : sharedCompound_(nullptr) {}
  explicit SharedCompound(std::nullptr_t) : sharedCompound_(nullptr) {}

  explicit SharedCompound(std::shared_ptr<CompoundType> ct)
      : sharedCompound_(std::move(ct)) {}

  void onSubscribe(AnySubscriptionRef subscriber) {
#ifndef NDEBUG
    CHECK(!wasSubscribed_);
    CHECK(!wasTermianted_);
    wasSubscribed_ = true;
#endif
    if (sharedCompound_) {
      sharedCompound_->onSubscribe(subscriber);
    }
  }

  void onNext(T elem) {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
#endif
    if (sharedCompound_) {
      sharedCompound_->onNext(std::move(elem));
    }
  }

  void onComplete() {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
    wasTermianted_ = true;
#endif

    if (auto s = sharedCompound_) {
      s->onComplete();
    }
  }

  void onError(folly::exception_wrapper err) {
#ifndef NDEBUG
    CHECK(wasSubscribed_);
    CHECK(!wasTermianted_);
    wasTermianted_ = true;
#endif

    if (auto s = sharedCompound_) {
      s->onError(std::move(err));
    }
  }

  void cancel() {
    if (sharedCompound_) {
      sharedCompound_->cancel();
    }
  }

  void request(int64_t n) {
    if (sharedCompound_) {
      sharedCompound_->request(n);
    }
  }

  CompoundType& underlying() const {
    CHECK(sharedCompound_);
    return *sharedCompound_;
  }

  bool cleared() const {
    return !!sharedCompound_;
  }
  void clear() {
    sharedCompound_.reset();
  }

 private:
#ifndef NDEBUG
  bool wasSubscribed_{false};
  bool wasTermianted_{false};
#endif
  std::shared_ptr<CompoundType> sharedCompound_;
};

template <typename Compound>
auto makeSharedCompound(Compound&& subscriber) {
  return SharedCompound<Compound>(std::move(subscriber));
}

} // namespace flowable
} // namespace erased
} // namespace yarpl
