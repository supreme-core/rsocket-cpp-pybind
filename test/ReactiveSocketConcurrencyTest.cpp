// Copyright 2004-present Facebook. All Rights Reserved.

#include <array>
#include <chrono>
#include <memory>
#include <thread>

#include <folly/Memory.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockStats.h"
#include "src/ReactiveSocket.h"
#include "test/InlineConnection.h"
#include "test/MockRequestHandler.h"
#include "test/ReactiveStreamsMocksCompat.h"

using namespace ::testing;
using namespace ::reactivesocket;

class ClientSideConcurrencyTest : public testing::Test {
 public:
  ClientSideConcurrencyTest() {
    auto clientConn = folly::make_unique<InlineConnection>();
    auto serverConn = folly::make_unique<InlineConnection>();
    clientConn->connectTo(*serverConn);

    clientSock = ReactiveSocket::fromClientConnection(
        std::move(clientConn),
        // No interactions on this mock, the client will not accept any
        // requests.
        folly::make_unique<StrictMock<MockRequestHandler>>(),
        ConnectionSetupPayload("", "", Payload()));

    auto serverHandler = folly::make_unique<StrictMock<MockRequestHandler>>();
    auto& serverHandlerRef = *serverHandler;

    EXPECT_CALL(serverHandlerRef, handleSetupPayload_(_));

    serverSock = ReactiveSocket::fromServerConnection(
        std::move(serverConn), std::move(serverHandler));

    EXPECT_CALL(clientInput, onSubscribe_(_))
        .WillOnce(Invoke([&](Subscription* sub) {
          clientInputSub = sub;
          // request is called from the thread1
          // but delivered on thread2
          thread1.getEventBase()->runInEventBaseThreadAndWait(
              [&]() { sub->request(2); });
        }));
    // The request reaches the other end and triggers new responder to be set
    // up.
    EXPECT_CALL(serverHandlerRef, handleRequestResponse_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput = &subscriberFactory.createSubscriber();
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestStream_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput = &subscriberFactory.createSubscriber();
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestSubscription_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput = &subscriberFactory.createSubscriber();
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestChannel_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              clientTerminatesInteraction_ = false;
              EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());

              EXPECT_CALL(serverInput, onSubscribe_(_))
                  .WillOnce(Invoke([&](Subscription* sub) {
                    EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
                    serverInputSub = sub;
                    sub->request(2);
                  }));
              EXPECT_CALL(serverInput, onNext_(_))
                  .WillOnce(Invoke([&](Payload& payload) {
                    EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
                    serverInputSub->cancel();
                  }));
              EXPECT_CALL(serverInput, onComplete_()).WillOnce(Invoke([&]() {
                EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
              }));

              serverOutput = &subscriberFactory.createSubscriber();
              serverOutput->onSubscribe(serverOutputSub);

              return &serverInput;
            }));

    EXPECT_CALL(serverOutputSub, request_(_))
        // The server delivers them immediately.
        .WillOnce(Invoke([&](size_t) {
          EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
          serverOutput->onNext(Payload(originalPayload->clone()));
        }));
    EXPECT_CALL(clientInput, onNext_(_))
        // Client receives the payload. We will complete the interaction
        .WillOnce(Invoke([&](Payload&) {
          // cancel is called from the thread1
          // but delivered on thread2
          if (clientTerminatesInteraction_) {
            thread1.getEventBase()->runInEventBaseThreadAndWait(
                [&]() { clientInputSub->cancel(); });
          }
        }));

    EXPECT_CALL(serverOutputSub, cancel_()).WillOnce(Invoke([&]() {
      EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
      serverOutput->onComplete();
    }));

    EXPECT_CALL(clientInput, onComplete_());
  }

  std::unique_ptr<ReactiveSocket> clientSock;
  std::unique_ptr<ReactiveSocket> serverSock;

  const std::unique_ptr<folly::IOBuf> originalPayload{
      folly::IOBuf::copyBuffer("foo")};

  StrictMock<UnmanagedMockSubscriber<Payload>> clientInput;
  Subscription* clientInputSub{nullptr};

  Subscriber<Payload>* serverOutput{nullptr};
  StrictMock<UnmanagedMockSubscription> serverOutputSub;

  StrictMock<UnmanagedMockSubscriber<Payload>> serverInput;
  Subscription* serverInputSub;

  folly::ScopedEventBaseThread thread1;
  folly::ScopedEventBaseThread thread2;

  bool clientTerminatesInteraction_{true};
};

TEST_F(ClientSideConcurrencyTest, RequestResponseTest) {
  clientSock->requestResponse(
      Payload(originalPayload->clone()), clientInput, *thread2.getEventBase());
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ClientSideConcurrencyTest, RequestStreamTest) {
  clientSock->requestStream(
      Payload(originalPayload->clone()), clientInput, *thread2.getEventBase());
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ClientSideConcurrencyTest, RequestSubscriptionTest) {
  clientSock->requestSubscription(
      Payload(originalPayload->clone()), clientInput, *thread2.getEventBase());
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ClientSideConcurrencyTest, RequestChannelTest) {
  auto& clientOutput =
      clientSock->requestChannel(clientInput, *thread2.getEventBase());

  StrictMock<UnmanagedMockSubscription> clientOutputSub;
  EXPECT_CALL(clientOutputSub, request_(1)).WillOnce(Invoke([&](size_t) {
    thread1.getEventBase()->runInEventBaseThread([&]() {
      // first payload for the server RequestHandler
      clientOutput.onNext(Payload(originalPayload->clone()));
    });
  }));
  EXPECT_CALL(clientOutputSub, request_(2)).WillOnce(Invoke([&](size_t) {
    // second payload for the server input subscriber
    clientOutput.onNext(Payload(originalPayload->clone()));
  }));
  EXPECT_CALL(clientOutputSub, cancel_()).WillOnce(Invoke([&]() {
    thread1.getEventBase()->runInEventBaseThread(
        [&]() { clientOutput.onComplete(); });
  }));

  thread1.getEventBase()->runInEventBaseThreadAndWait(
      [&]() { clientOutput.onSubscribe(clientOutputSub); });

  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

class ServerSideConcurrencyTest : public testing::Test {
 public:
  ServerSideConcurrencyTest() {
    auto clientConn = folly::make_unique<InlineConnection>();
    auto serverConn = folly::make_unique<InlineConnection>();
    clientConn->connectTo(*serverConn);

    clientSock = ReactiveSocket::fromClientConnection(
        std::move(clientConn),
        // No interactions on this mock, the client will not accept any
        // requests.
        folly::make_unique<StrictMock<MockRequestHandler>>(),
        ConnectionSetupPayload("", "", Payload()));

    auto serverHandler = folly::make_unique<StrictMock<MockRequestHandler>>();
    auto& serverHandlerRef = *serverHandler;

    EXPECT_CALL(serverHandlerRef, handleSetupPayload_(_));

    serverSock = ReactiveSocket::fromServerConnection(
        std::move(serverConn), std::move(serverHandler));

    EXPECT_CALL(clientInput, onSubscribe_(_))
        .WillOnce(Invoke([&](Subscription* sub) {
          clientInputSub = sub;
          sub->request(2);
        }));
    // The request reaches the other end and triggers new responder to be set
    // up.
    EXPECT_CALL(serverHandlerRef, handleRequestResponse_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput =
                  &subscriberFactory.createSubscriber(*thread2.getEventBase());
              // TODO: should onSubscribe be queued and also called from
              // thread1?
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestStream_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput =
                  &subscriberFactory.createSubscriber(*thread2.getEventBase());
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestSubscription_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              serverOutput =
                  &subscriberFactory.createSubscriber(*thread2.getEventBase());
              serverOutput->onSubscribe(serverOutputSub);
            }));
    EXPECT_CALL(serverHandlerRef, handleRequestChannel_(_, _))
        .Times(AtMost(1))
        .WillOnce(
            Invoke([&](Payload& request, SubscriberFactory& subscriberFactory) {
              clientTerminatesInteraction_ = false;

              EXPECT_CALL(serverInput, onSubscribe_(_))
                  .WillOnce(Invoke([&](Subscription* sub) {
                    serverInputSub = sub;
                    thread1.getEventBase()->runInEventBaseThreadAndWait(
                        [&]() { sub->request(2); });
                  }));
              EXPECT_CALL(serverInput, onNext_(_))
                  .WillOnce(Invoke([&](Payload& payload) {
                    thread1.getEventBase()->runInEventBaseThreadAndWait(
                        [&]() { serverInputSub->cancel(); });
                  }));
              EXPECT_CALL(serverInput, onComplete_()).WillOnce(Invoke([&]() {
                EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
              }));

              serverOutput =
                  &subscriberFactory.createSubscriber(*thread2.getEventBase());
              serverOutput->onSubscribe(serverOutputSub);

              return &serverInput;
            }));

    EXPECT_CALL(serverOutputSub, request_(_))
        // The server delivers them immediately.
        .WillOnce(Invoke([&](size_t) {
          thread1.getEventBase()->runInEventBaseThreadAndWait([&]() {
            serverOutput->onNext(Payload(originalPayload->clone()));
          });
        }));
    EXPECT_CALL(clientInput, onNext_(_))
        // Client receives the payload. We will complete the interaction
        .WillOnce(Invoke([&](Payload&) {
          EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
          // cancel is called from the thread1
          // but delivered on thread2
          if (clientTerminatesInteraction_) {
            clientInputSub->cancel();
          }
        }));

    EXPECT_CALL(serverOutputSub, cancel_()).WillOnce(Invoke([&]() {
      EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
      serverOutput->onComplete();
    }));

    EXPECT_CALL(clientInput, onComplete_());
  }

  std::unique_ptr<ReactiveSocket> clientSock;
  std::unique_ptr<ReactiveSocket> serverSock;

  const std::unique_ptr<folly::IOBuf> originalPayload{
      folly::IOBuf::copyBuffer("foo")};

  StrictMock<UnmanagedMockSubscriber<Payload>> clientInput;
  Subscription* clientInputSub{nullptr};

  Subscriber<Payload>* serverOutput{nullptr};
  StrictMock<UnmanagedMockSubscription> serverOutputSub;

  StrictMock<UnmanagedMockSubscriber<Payload>> serverInput;
  Subscription* serverInputSub;

  folly::ScopedEventBaseThread thread1;
  folly::ScopedEventBaseThread thread2;

  bool clientTerminatesInteraction_{true};
};

TEST_F(ServerSideConcurrencyTest, RequestResponseTest) {
  clientSock->requestResponse(Payload(originalPayload->clone()), clientInput);
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ServerSideConcurrencyTest, RequestStreamTest) {
  clientSock->requestStream(Payload(originalPayload->clone()), clientInput);
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ServerSideConcurrencyTest, RequestSubscriptionTest) {
  clientSock->requestSubscription(
      Payload(originalPayload->clone()), clientInput);
  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(ServerSideConcurrencyTest, RequestChannelTest) {
  auto& clientOutput = clientSock->requestChannel(clientInput);

  StrictMock<UnmanagedMockSubscription> clientOutputSub;
  EXPECT_CALL(clientOutputSub, request_(1)).WillOnce(Invoke([&](size_t n) {
    // first payload for the server RequestHandler
    clientOutput.onNext(Payload(originalPayload->clone()));
  }));
  EXPECT_CALL(clientOutputSub, request_(2)).WillOnce(Invoke([&](size_t n) {
    EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
    // second payload for the server input subscriber
    clientOutput.onNext(Payload(originalPayload->clone()));
  }));
  EXPECT_CALL(clientOutputSub, cancel_()).WillOnce(Invoke([&]() {
    EXPECT_TRUE(thread2.getEventBase()->isInEventBaseThread());
    clientOutput.onComplete();
  }));

  clientOutput.onSubscribe(clientOutputSub);

  // give the second thread a change to execute the code
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
