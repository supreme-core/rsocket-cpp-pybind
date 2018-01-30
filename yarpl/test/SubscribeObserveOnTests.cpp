// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <gtest/gtest.h>
#include <thread>
#include <type_traits>
#include <vector>

#include "yarpl/Flowable.h"
#include "yarpl/flowable/TestSubscriber.h"
#include "yarpl/test_utils/utils.h"

namespace yarpl {
namespace flowable {
namespace {

TEST(ObserveSubscribeTests, SubscribeOnWorksAsExpected) {
  folly::ScopedEventBaseThread worker;

  auto f = Flowable<std::string>::create([&](auto& subscriber, auto req) {
    EXPECT_TRUE(worker.getEventBase()->isInEventBaseThread());
    EXPECT_EQ(1, req);
    subscriber.onNext("foo");
    subscriber.onComplete();
  });

  auto subscriber = std::make_shared<TestSubscriber<std::string>>(1);
  f->subscribeOn(*worker.getEventBase())->subscribe(subscriber);
  subscriber->awaitTerminalEvent(std::chrono::milliseconds(100));
  EXPECT_EQ(1, subscriber->getValueCount());
  EXPECT_TRUE(subscriber->isComplete());
}

TEST(ObserveSubscribeTests, ObserveOnWorksAsExpectedSuccess) {
  folly::ScopedEventBaseThread worker;
  folly::Baton<> subscriber_complete;

  auto f = Flowable<std::string>::create([&](auto& subscriber, auto req) {
    EXPECT_EQ(1, req);
    subscriber.onNext("foo");
    subscriber.onComplete();
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

  auto f = Flowable<std::string>::create([&](auto& subscriber, auto req) {
    EXPECT_EQ(1, req);
    subscriber.onError(std::runtime_error("oops!"));
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

  auto f = Flowable<std::string>::create([&](auto& subscriber, auto req) {
             EXPECT_EQ(1, req);
             EXPECT_TRUE(producer_eb.getEventBase()->isInEventBaseThread());
             subscriber.onNext("foo");
             subscriber.onComplete();
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
class EarlyCancelSubscriber : public yarpl::flowable::BaseSubscriber<int64_t> {
 public:
  EarlyCancelSubscriber(
      folly::EventBase& on_base,
      folly::Baton<>& subscriber_complete)
      : on_base_(on_base), subscriber_complete_(subscriber_complete) {}

  void onSubscribeImpl() override {
    this->request(5);
  }

  void onNextImpl(int64_t n) override {
    if (did_cancel_) {
      FAIL();
    }

    EXPECT_TRUE(on_base_.isInEventBaseThread());
    EXPECT_EQ(n, 1);
    this->cancel();
    did_cancel_ = true;
    subscriber_complete_.post();
  }

  void onErrorImpl(folly::exception_wrapper e) override {
    FAIL();
  }

  void onCompleteImpl() override {
    FAIL();
  }

  bool did_cancel_{false};
  folly::EventBase& on_base_;
  folly::Baton<>& subscriber_complete_;
};
} // namespace

TEST(ObserveSubscribeTests, EarlyCancelObserveOn) {
  folly::ScopedEventBaseThread worker;

  folly::Baton<> subscriber_complete;

  Flowable<>::range(1, 100)
      ->observeOn(*worker.getEventBase())
      ->subscribe(std::make_shared<EarlyCancelSubscriber>(
          *worker.getEventBase(), subscriber_complete));

  CHECK_WAIT(subscriber_complete);
}
} // namespace
} // namespace flowable
} // namespace yarpl
