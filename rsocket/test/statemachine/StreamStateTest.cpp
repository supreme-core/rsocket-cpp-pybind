// Copyright 2004-present Facebook. All Rights Reserved.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <yarpl/test_utils/mocks.h>

#include "rsocket/internal/Common.h"
#include "rsocket/statemachine/ChannelRequester.h"
#include "rsocket/statemachine/ChannelResponder.h"
#include "rsocket/statemachine/StreamStateMachineBase.h"
#include "rsocket/test/test_utils/MockStreamsWriter.h"

using namespace rsocket;
using namespace testing;
using namespace yarpl::mocks;

TEST(StreamState, NewStateMachineBase) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  EXPECT_CALL(*writer, onStreamClosed(_));

  StreamStateMachineBase ssm(writer, 1u);
  ssm.getConsumerAllowance();
  ssm.handleCancel();
  ssm.handleError(std::runtime_error("test"));
  ssm.handlePayload(Payload{}, false, true);
  ssm.handleRequestN(1);
}

TEST(StreamState, ChannelRequesterOnError) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto requester = std::make_shared<ChannelRequester>(writer, 1u);

  EXPECT_CALL(*writer, writeNewStream(1u, _, _, _));
  EXPECT_CALL(*writer, writeError(_));
  EXPECT_CALL(*writer, onStreamClosed(1u));

  auto subscription = std::make_shared<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, cancel_()).Times(0);
  EXPECT_CALL(*subscription, request_(1));

  auto mockSubscriber =
      std::make_shared<StrictMock<MockSubscriber<rsocket::Payload>>>();
  EXPECT_CALL(*mockSubscriber, onSubscribe_(_));
  EXPECT_CALL(*mockSubscriber, onError_(_));
  requester->subscribe(mockSubscriber);

  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(subscription);

  // Initial request to activate the channel
  subscriber->onNext(Payload());

  ASSERT_FALSE(requester->consumerClosed());
  ASSERT_FALSE(requester->publisherClosed());

  subscriber->onError(std::runtime_error("test"));

  ASSERT_TRUE(requester->publisherClosed());
  ASSERT_TRUE(requester->consumerClosed());
}

TEST(StreamState, ChannelResponderOnError) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto responder = std::make_shared<ChannelResponder>(writer, 1u, 0u);

  EXPECT_CALL(*writer, writeError(_));
  EXPECT_CALL(*writer, onStreamClosed(1u));
  EXPECT_CALL(*writer, writeRequestN(_));

  auto mockSubscriber =
      std::make_shared<StrictMock<MockSubscriber<rsocket::Payload>>>();
  EXPECT_CALL(*mockSubscriber, onSubscribe_(_));
  EXPECT_CALL(*mockSubscriber, onError_(_));
  responder->subscribe(mockSubscriber);

  auto subscription = std::make_shared<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, cancel_()).Times(0);
  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = responder.get();
  subscriber->onSubscribe(subscription);

  ASSERT_FALSE(responder->consumerClosed());
  ASSERT_FALSE(responder->publisherClosed());

  subscriber->onError(std::runtime_error("test"));

  ASSERT_TRUE(responder->publisherClosed());
  ASSERT_TRUE(responder->consumerClosed());
}

TEST(StreamState, ChannelRequesterHandleError) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto requester = std::make_shared<ChannelRequester>(writer, 1u);

  EXPECT_CALL(*writer, writeNewStream(1u, _, _, _));
  EXPECT_CALL(*writer, writeError(_)).Times(0);
  EXPECT_CALL(*writer, onStreamClosed(1u)).Times(0);

  auto mockSubscriber =
      std::make_shared<StrictMock<MockSubscriber<rsocket::Payload>>>();
  EXPECT_CALL(*mockSubscriber, onSubscribe_(_));
  EXPECT_CALL(*mockSubscriber, onError_(_));
  requester->subscribe(mockSubscriber);

  auto subscription = std::make_shared<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, cancel_());
  EXPECT_CALL(*subscription, request_(1));

  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(subscription);
  // Initial request to activate the channel
  subscriber->onNext(Payload());

  ASSERT_FALSE(requester->consumerClosed());
  ASSERT_FALSE(requester->publisherClosed());

  ConsumerBase* consumer = requester.get();
  consumer->handleError(std::runtime_error("test"));

  ASSERT_TRUE(requester->consumerClosed());
  ASSERT_TRUE(requester->publisherClosed());
}

TEST(StreamState, ChannelResponderHandleError) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto responder = std::make_shared<ChannelResponder>(writer, 1u, 0u);

  EXPECT_CALL(*writer, writeError(_)).Times(0);
  EXPECT_CALL(*writer, onStreamClosed(1u)).Times(0);
  EXPECT_CALL(*writer, writeRequestN(_));

  auto mockSubscriber =
      std::make_shared<StrictMock<MockSubscriber<rsocket::Payload>>>();
  EXPECT_CALL(*mockSubscriber, onSubscribe_(_));
  EXPECT_CALL(*mockSubscriber, onError_(_));

  responder->subscribe(mockSubscriber);

  // Initialize the responder
  auto subscription = std::make_shared<StrictMock<MockSubscription>>();
  EXPECT_CALL(*subscription, cancel_());
  EXPECT_CALL(*subscription, request_(1)).Times(0);

  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = responder.get();
  subscriber->onSubscribe(subscription);

  ASSERT_FALSE(responder->consumerClosed());
  ASSERT_FALSE(responder->publisherClosed());

  ConsumerBase* consumer = responder.get();
  consumer->handleError(std::runtime_error("test"));

  ASSERT_TRUE(responder->publisherClosed());
  ASSERT_TRUE(responder->consumerClosed());
}
