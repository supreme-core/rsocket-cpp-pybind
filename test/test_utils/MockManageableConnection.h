// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gmock/gmock.h>

#include <rsocket/internal/ManageableConnection.h>

namespace rsocket {

class MockManageableConnection : public rsocket::ManageableConnection {
public:
  MOCK_METHOD0(listenCloseEvent_, folly::Future<folly::Unit>());
  MOCK_METHOD1(onClose_, void(folly::exception_wrapper));
  MOCK_METHOD2(close_, void(folly::exception_wrapper, StreamCompletionSignal));

  folly::Future<folly::Unit> listenCloseEvent() noexcept {
    listenCloseEvent_();
    return closePromise_.getFuture();
  }

  /// Terminates underlying connection.
  ///
  /// This may synchronously deliver terminal signals to all
  /// StreamAutomatonBase attached to this ConnectionAutomaton.
  void close(folly::exception_wrapper ex,
             StreamCompletionSignal scs) noexcept override {
    close_(ex, scs);
    onClose(ex);
  }

protected:
  void onClose(folly::exception_wrapper ex) noexcept {
    onClose_(ex);
    closePromise_.setValue();
  }
};

} // namespace rsocket
