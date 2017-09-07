// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include <folly/io/async/EventBase.h>
#include <rsocket/internal/Common.h>
#include <rsocket/internal/RSocketConnectionManager.h>
#include <test/test_utils/MockManageableConnection.h>

using namespace rsocket;
using namespace testing;

TEST(RSocketConnectionManagerTest, None) {
  RSocketConnectionManager conMgr;
  StrictMock<MockManageableConnection> con;
}

TEST(RSocketConnectionManagerTest, TerminateConnectionManager) {
  auto spCon = std::make_shared<StrictMock<MockManageableConnection>>();
  EXPECT_CALL(*spCon, onClose_(_));
  EXPECT_CALL(*spCon, close_(_, StreamCompletionSignal::SOCKET_CLOSED));
  folly::EventBase evb;
  {
    RSocketConnectionManager conMgr;
    conMgr.manageConnection(spCon, evb);
  }
}

TEST(RSocketConnectionManagerTest, TerminateConnectionWithDestruction) {
  folly::EventBase evb;
  {
    RSocketConnectionManager conMgr;
    {
      auto spCon = std::make_shared<StrictMock<MockManageableConnection>>();
      EXPECT_CALL(*spCon, onClose_(_));
      EXPECT_CALL(*spCon, close_(_, StreamCompletionSignal::SOCKET_CLOSED));
      conMgr.manageConnection(spCon, evb);
    }
  }
}

TEST(RSocketConnectionManagerTest, NonManagedConnection) {
  auto spCon = std::make_shared<StrictMock<MockManageableConnection>>();
  // no one is calling close_ or onClose_
}

TEST(RSocketConnectionManagerTest, NonManagedConnectionClosesItself) {
  auto spCon = std::make_shared<StrictMock<MockManageableConnection>>();
  EXPECT_CALL(*spCon, onClose_(_));
  EXPECT_CALL(*spCon, close_(_, StreamCompletionSignal::CANCEL));
  spCon->close(folly::exception_wrapper(), StreamCompletionSignal::CANCEL);
}

TEST(RSocketConnectionManagerTest, ManagedConnectionClosesItself) {
  RSocketConnectionManager conMgr;
  folly::EventBase evb;
  {
    auto spCon = std::make_shared<StrictMock<MockManageableConnection>>();
    EXPECT_CALL(*spCon, onClose_(_));
    EXPECT_CALL(*spCon, close_(_, StreamCompletionSignal::CANCEL));
    conMgr.manageConnection(spCon, evb);
    spCon->close(folly::exception_wrapper(), StreamCompletionSignal::CANCEL);
  }
}
