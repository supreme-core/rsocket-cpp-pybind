// Copyright 2004-present Facebook. All Rights Reserved.

#include "yarpl/flowable/Subscriber.h"
#include "yarpl/test_utils/Mocks.h"

using namespace yarpl;
using namespace yarpl::flowable;
using namespace yarpl::mocks;
using namespace testing;

namespace {

TEST(FlowableSubscriberTest, TestBasicFunctionality) {
  Sequence subscriber_seq;
  auto subscriber = yarpl::make_ref<StrictMock<MockSafeSubscriber<int>>>();

  EXPECT_CALL(*subscriber, onSubscribeImpl())
    .Times(1)
    .InSequence(subscriber_seq)
    .WillOnce(Invoke([&] {
      subscriber->request(3);
    }));
  EXPECT_CALL(*subscriber, onNextImpl(5))
    .Times(1)
    .InSequence(subscriber_seq);
  EXPECT_CALL(*subscriber, onCompleteImpl())
    .Times(1)
    .InSequence(subscriber_seq);

  auto subscription = yarpl::make_ref<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, request_(3))
    .Times(1)
    .WillOnce(InvokeWithoutArgs([&] {
      subscriber->onNext(5);
      subscriber->onComplete();
    }));

  subscriber->onSubscribe(subscription);
}

}
