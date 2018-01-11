// Copyright 2004-present Facebook. All Rights Reserved.

#include "RSocketTests.h"

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>

#include "rsocket/transports/tcp/TcpConnectionFactory.h"
#include "rsocket/test/test_utils/MockDuplexConnection.h"

using namespace rsocket;
using namespace rsocket::tests;
using namespace rsocket::tests::client_server;
using namespace testing;
using namespace yarpl::single;

TEST(RSocketClient, ConnectFails) {
  folly::ScopedEventBaseThread worker;

  folly::SocketAddress address;
  address.setFromHostPort("localhost", 1);
  auto client = RSocket::createConnectedClient(
      std::make_unique<TcpConnectionFactory>(*worker.getEventBase(),
                                             std::move(address)));

  client.then([&](auto&) {
    FAIL() << "the test needs to fail";
  }).onError([&](const std::exception&) {
    LOG(INFO) << "connection failed as expected";
  }).get();
}

TEST(RSocketClient, PreallocatedBytesInFrames) {
  auto connection = std::make_unique<MockDuplexConnection>();
  EXPECT_CALL(*connection, isFramed()).WillRepeatedly(Return(true));

  // SETUP frame and FIRE_N_FORGET frame send
  EXPECT_CALL(*connection, send_(_))
      .Times(2)
      .WillRepeatedly(
          Invoke([](std::unique_ptr<folly::IOBuf>& serializedFrame) {
            // we should have headroom preallocated for the frame size field
            EXPECT_EQ(
                FrameSerializer::createFrameSerializer(
                    ProtocolVersion::Current())
                    ->frameLengthFieldSize(),
                serializedFrame->headroom());
          }));

  folly::ScopedEventBaseThread worker;

  worker.getEventBase()->runInEventBaseThread([&] {
    auto client = RSocket::createClientFromConnection(
        std::move(connection), *worker.getEventBase());

    client->getRequester()->fireAndForget(Payload("hello"))->subscribe(
    SingleObservers::create<void>());
  });
}
