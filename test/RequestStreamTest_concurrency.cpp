// Copyright 2004-present Facebook. All Rights Reserved.

#include <thread>
#include <iostream>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>

#include "RSocketTests.h"
#include "yarpl/Flowable.h"
#include "yarpl/flowable/TestSubscriber.h"

#include "yarpl/test_utils/Mocks.h"

using namespace yarpl;
using namespace yarpl::flowable;
using namespace rsocket;
using namespace rsocket::tests;
using namespace rsocket::tests::client_server;

struct LockstepBatons {
  folly::Baton<> on_second_payload_sent;
  folly::Baton<> on_cancel_sent;
  folly::Baton<> on_cancel_recieved_toserver;
  folly::Baton<> on_cancel_recieved_toclient;
  folly::Baton<> on_request_recieved;
  folly::Baton<> client_finished;
  folly::Baton<> server_finished;
};

using namespace yarpl::mocks;
using namespace ::testing;

#define LOCKSTEP_DEBUG(expr) VLOG(3) << expr

class LockstepAsyncHandler : public rsocket::RSocketResponder {
  LockstepBatons& batons_;
  Sequence& subscription_seq_;
 public:
  LockstepAsyncHandler(LockstepBatons& batons, Sequence& subscription_seq)
    : batons_(batons), subscription_seq_(subscription_seq) {};

  Reference<Flowable<Payload>> handleRequestStream(Payload p, StreamId) override {
    EXPECT_EQ(p.moveDataToString(), "initial");
    return Flowables::fromPublisher<Payload>(
      [this](Reference<flowable::Subscriber<Payload>> subscriber)
      {
        auto subscription = make_ref<StrictMock<MockSubscription>>();

        std::thread([=] {
            CHECK_WAIT(this->batons_.on_request_recieved);

            LOCKSTEP_DEBUG("SERVER: sending onNext(foo)");
            subscriber->onNext(Payload("foo"));
            CHECK_WAIT(this->batons_.on_cancel_sent);
            CHECK_WAIT(this->batons_.on_cancel_recieved_toserver);

            LOCKSTEP_DEBUG("SERVER: sending onNext(bar)");
            subscriber->onNext(Payload("bar"));
            this->batons_.on_second_payload_sent.post();
            LOCKSTEP_DEBUG("SERVER: sending onComplete()");
            subscriber->onComplete();
            LOCKSTEP_DEBUG("SERVER: posting server_finished");
            this->batons_.server_finished.post();
        }).detach();

        // checked once the subscription is destroyed
        EXPECT_CALL(*subscription, request_(2))
          .InSequence(this->subscription_seq_)
          .WillOnce(Invoke([=](auto n) {
            LOCKSTEP_DEBUG("SERVER: got request(" << n << ")");
            EXPECT_EQ(n, 2);
            this->batons_.on_request_recieved.post();
          }));

        EXPECT_CALL(*subscription, cancel_())
          .InSequence(this->subscription_seq_)
          .WillOnce(Invoke([=]{
            LOCKSTEP_DEBUG("SERVER: recieved cancel()");
              this->batons_.on_cancel_recieved_toclient.post();
              this->batons_.on_cancel_recieved_toserver.post();
          }));

        LOCKSTEP_DEBUG("SERVER: sending onSubscribe()");
        subscriber->onSubscribe(subscription);
      });
  }
};

TEST(RequestStreamTest, OperationsAfterCancel) {
  LockstepBatons batons;
  Sequence server_seq;
  Sequence client_seq;

  auto server = makeServer(std::make_shared<LockstepAsyncHandler>(batons, server_seq));
  folly::ScopedEventBaseThread worker;
  auto client = makeClient(worker.getEventBase(), *server->listeningPort());
  auto requester = client->getRequester();

  auto scbr_mock = make_ref<testing::StrictMock<yarpl::mocks::MockSubscriber<std::string>>>(0);

  Reference<Subscription> subscription;
  EXPECT_CALL(*scbr_mock, onSubscribe_(_))
    .InSequence(client_seq)
    .WillOnce(Invoke([&](auto s) {
      LOCKSTEP_DEBUG("CLIENT: got onSubscribe(), sending request(2)");
      EXPECT_NE(s, nullptr);
      subscription = s;
      subscription->request(2);
    }));
  EXPECT_CALL(*scbr_mock, onNext_("foo"))
    .InSequence(client_seq)
    .WillOnce(Invoke([&](auto) {
      EXPECT_NE(subscription, nullptr);
      LOCKSTEP_DEBUG("CLIENT: got onNext(foo), sending cancel()");
      subscription->cancel();
      batons.on_cancel_sent.post();
      CHECK_WAIT(batons.on_cancel_recieved_toclient);
      CHECK_WAIT(batons.on_second_payload_sent);
    }));

  // shouldn't recieve 'bar', we canceled syncronously with the Subscriber
  // had 'cancel' been called in a different thread with no synchronization,
  // the client's Subscriber _could_ have recieved 'bar'

  EXPECT_CALL(*scbr_mock, onComplete_())
    .InSequence(client_seq)
    .WillOnce(Invoke([&]() {
      LOCKSTEP_DEBUG("CLIENT: got onComplete()");
      batons.client_finished.post();
    }));

  LOCKSTEP_DEBUG("RUNNER: doing requestStream()");
  requester->requestStream(Payload("initial"))
      ->map([](auto p) { return p.moveDataToString(); })
      ->subscribe(scbr_mock);

  CHECK_WAIT(batons.client_finished);
  CHECK_WAIT(batons.server_finished);
  LOCKSTEP_DEBUG("RUNNER: finished!");
}
