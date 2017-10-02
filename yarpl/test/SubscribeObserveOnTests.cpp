// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>
#include <thread>
#include <type_traits>
#include <vector>

#include <folly/Baton.h>

#include "yarpl/test_utils/utils.h"

#include "yarpl/Flowable.h"
#include "yarpl/flowable/TestSubscriber.h"

namespace yarpl {
namespace flowable {
namespace {

TEST(ObserveSubscribeTests, SubscribeOnWorksAsExpected) {
  folly::ScopedEventBaseThread worker;

  auto f = Flowable<std::string>::create([&](auto subscriber, auto req) {
    EXPECT_TRUE(worker.getEventBase()->isInEventBaseThread());
    EXPECT_EQ(1, req);
    subscriber->onNext("foo");
    subscriber->onComplete();
    return std::tuple<int64_t, bool>(1, true);
  });

  auto subscriber = make_ref<TestSubscriber<std::string>>(1);
  f->subscribeOn(*worker.getEventBase())->subscribe(subscriber);
  subscriber->awaitTerminalEvent(std::chrono::milliseconds(100));
  EXPECT_EQ(1, subscriber->getValueCount());
  EXPECT_TRUE(subscriber->isComplete());
}

TEST(ObserveSubscribeTests, ObserveOnWorksAsExpectedSuccess) {
  folly::ScopedEventBaseThread worker;
  folly::Baton<> subscriber_complete;

  auto f = Flowable<std::string>::create([&](auto subscriber, auto req) {
    EXPECT_EQ(1, req);
    subscriber->onNext("foo");
    subscriber->onComplete();
    return std::tuple<int64_t, bool>(1, true);
  });

  bool calledOnNext{false};

  f->observeOn(*worker.getEventBase())
      ->subscribe(
          // onNext
          [&](std::string s) {
            EXPECT_TRUE(worker.getEventBase()->isInEventBaseThread());
            EXPECT_EQ(s, "foo");
            calledOnNext = true;
          },

          // onError
          [&](folly::exception_wrapper) { FAIL(); },

          // onComplete
          [&] {
            EXPECT_TRUE(worker.getEventBase()->isInEventBaseThread());
            EXPECT_TRUE(calledOnNext);
            subscriber_complete.post();
          },

          1 /* initial request(n) */
          );

  CHECK_WAIT(subscriber_complete);
}

TEST(ObserveSubscribeTests, ObserveOnWorksAsExpectedError) {
  folly::ScopedEventBaseThread worker;
  folly::Baton<> subscriber_complete;

  auto f = Flowable<std::string>::create([&](auto subscriber, auto req) {
    EXPECT_EQ(1, req);
    subscriber->onError(std::runtime_error("oops!"));
    return std::tuple<int64_t, bool>(0, true);
  });

  f->observeOn(*worker.getEventBase())
      ->subscribe(
          // onNext
          [&](std::string s) { FAIL(); },

          // onError
          [&](folly::exception_wrapper) {
            EXPECT_TRUE(worker.getEventBase()->isInEventBaseThread());
            subscriber_complete.post();
          },

          // onComplete
          [&] { FAIL(); },

          1 /* initial request(n) */
          );

  CHECK_WAIT(subscriber_complete);
}

TEST(ObserveSubscribeTests, BothObserveAndSubscribeOn) {
  folly::ScopedEventBaseThread subscriber_eb;
  folly::ScopedEventBaseThread producer_eb;
  folly::Baton<> subscriber_complete;

  auto f = Flowable<std::string>::create([&](auto subscriber, auto req) {
             EXPECT_EQ(1, req);
             EXPECT_TRUE(producer_eb.getEventBase()->isInEventBaseThread());
             subscriber->onNext("foo");
             subscriber->onComplete();
             return std::tuple<int64_t, bool>(1, true);
           })
               ->subscribeOn(*producer_eb.getEventBase())
               ->observeOn(*subscriber_eb.getEventBase());

  bool calledOnNext{false};

  f->subscribe(
      // onNext
      [&](std::string s) {
        EXPECT_TRUE(subscriber_eb.getEventBase()->isInEventBaseThread());
        EXPECT_EQ(s, "foo");
        calledOnNext = true;
      },

      // onError
      [&](folly::exception_wrapper) { FAIL(); },

      // onComplete
      [&] {
        EXPECT_TRUE(subscriber_eb.getEventBase()->isInEventBaseThread());
        EXPECT_TRUE(calledOnNext);
        subscriber_complete.post();
      },

      1 /* initial request(n) */
      );

  CHECK_WAIT(subscriber_complete);
}

namespace {
class EarlyCancelSubscriber
    : public yarpl::flowable::InternalSubscriber<int64_t> {
 public:
  EarlyCancelSubscriber(
      folly::EventBase& on_base,
      folly::Baton<>& subscriber_complete)
      : on_base_(on_base), subscriber_complete_(subscriber_complete) {}

  void onSubscribe(Reference<yarpl::flowable::Subscription> s) override {
    InternalSubscriber::onSubscribe(s);
    s->request(5);
  }

  void onNext(int64_t n) override {
    if (did_cancel_) {
      FAIL();
    }

    EXPECT_TRUE(on_base_.isInEventBaseThread());
    EXPECT_EQ(n, 1);
    subscription()->cancel();
    did_cancel_ = true;
    subscriber_complete_.post();
  }

  void onError(folly::exception_wrapper e) override {
    FAIL();
  }

  void onComplete() override {
    FAIL();
  }

  bool did_cancel_{false};
  folly::EventBase& on_base_;
  folly::Baton<>& subscriber_complete_;
};
}

TEST(ObserveSubscribeTests, EarlyCancelObserveOn) {
  folly::ScopedEventBaseThread worker;

  folly::Baton<> subscriber_complete;

  Flowables::range(1, 100)
      ->observeOn(*worker.getEventBase())
      ->subscribe(make_ref<EarlyCancelSubscriber>(
          *worker.getEventBase(), subscriber_complete));

  CHECK_WAIT(subscriber_complete);
}
}
}
} /* namespace yarpl::flowable::<anon> */
