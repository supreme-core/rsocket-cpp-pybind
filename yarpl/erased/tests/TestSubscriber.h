#pragma once

#include <glog/logging.h>
#include "yarpl/erased/flowable/Flowable.h"

namespace yarpl {
namespace erased {
namespace flowable {

template <typename T>
struct TestSubscriber {
  TestSubscriber(int64_t initialRequest) : initialRequest(initialRequest) {}

  // move-only type
  TestSubscriber(TestSubscriber&&) = default;
  TestSubscriber(TestSubscriber const&) = delete;

  void onSubscribe(strm::flowable::AnySubscriptionRef subscription) {
    CHECK(!wasSubscribed);
    wasSubscribed = true;
    // TODO: gcc doesn't like subscription->request
    subscription.get().request(initialRequest);
  }

  void onNext(T elem) {
    CHECK(wasSubscribed);
    elems.push_back(std::move(elem));
  }

  void onComplete() {
    CHECK(wasSubscribed);
    CHECK(!wasCompleted);
    CHECK(!wasErrored);
    wasCompleted = true;
  }

  void onError(folly::exception_wrapper given) {
    CHECK(wasSubscribed);
    CHECK(!wasCompleted);
    CHECK(!wasErrored);
    wasErrored = true;
    ex = std::move(given);
  }

  int64_t initialRequest{0};
  bool wasSubscribed{false};

  std::vector<T> elems;

  bool wasCompleted{false};

  bool wasErrored{false};
  folly::exception_wrapper ex{};
};

} // namespace flowable
} // namespace erased
} // namespace yarpl
