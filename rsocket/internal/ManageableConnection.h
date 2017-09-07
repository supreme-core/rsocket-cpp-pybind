// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <folly/ExceptionWrapper.h>
#include <folly/futures/SharedPromise.h>
#include <folly/io/async/EventBase.h>

namespace rsocket {

class MockManageableConnection;
enum class StreamCompletionSignal;

/// Utility class that enables RSocketConnectionManager to handle
/// lifetime of RSocketStateMachine objects.
class ManageableConnection {
public:
  virtual ~ManageableConnection() {
    if (!closePromise_.isFulfilled()) {
      // don't let any object indefinitely wait for the close event.
      closePromise_.setValue();
    }
  }

  /// Listen for the close event.
  ///
  /// The proposed usage is utilizing `via` function and appending
  /// more functionality with `then` function.
  ///
  /// \return a Future to bind to.
  folly::Future<folly::Unit> listenCloseEvent() {
    return closePromise_.getFuture();
  }

  /// Terminates underlying connection.
  ///
  /// This may synchronously deliver terminal signals to all
  /// StreamAutomatonBase attached to this ConnectionAutomaton.
  virtual void close(folly::exception_wrapper,
                     StreamCompletionSignal) = 0;

protected:
  void onClose(folly::exception_wrapper) {
    // ignore the exception
    closePromise_.setValue();
  }

private:
  folly::SharedPromise<folly::Unit> closePromise_;

  friend MockManageableConnection;
};

} // namespace rsocket
