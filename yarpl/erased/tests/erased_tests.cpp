#include <glog/logging.h>
#include <gtest/gtest.h>
#include <yarpl/erased/flowable/Flowable.h>

namespace yarpl {
namespace erased {
namespace flowable {

struct my_subscription {
  my_subscription(my_subscription const&) = delete;
  my_subscription(my_subscription&&) = default;

  void cancel() {}
  void request(int64_t n) {}
};

template <typename T>
struct my_subscriber {
  my_subscriber(my_subscriber const&) = delete;
  my_subscriber(my_subscriber&&) = default;

  void onSubscribe(AnySubscriptionRef subscription) {
    subscription.get().request(1);
  }
  void onNext(T) {}
  void onComplete() {}
  void onError(folly::exception_wrapper) {}
};

TEST(AnyFlowable, CanCompile) {
  AnySubscriber<int> mything = my_subscriber<int>{};
  AnySubscription mysub = EmptySubscription{};
  mything.onSubscribe(mysub);
}

TEST(AnyFlowable, CanMakeFlowable) {
  auto flowable = makeFlowable<int>([](auto subscriber) {
    AnySubscription subscription = EmptySubscription{};
    subscriber.onSubscribe(subscription);
    subscriber.onComplete();
    return subscription;
  });

  auto subscriber = my_subscriber<int>{};
  auto subscription = flowable.subscribe(std::move(subscriber));
  subscription.cancel();
}

} // namespace flowable
} // namespace erased
} // namespace yarpl
