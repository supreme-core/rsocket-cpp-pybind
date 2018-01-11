// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <unordered_set>

#include <folly/Function.h>
#include <folly/futures/Future.h>

#include "rsocket/DuplexConnection.h"
#include "rsocket/RSocketParameters.h"
#include "yarpl/Refcounted.h"

namespace folly {
class EventBase;
class Executor;
class IOBuf;
class exception_wrapper;
} // namespace folly

namespace rsocket {

/// Acceptor of DuplexConnections that lets us decide whether the connection is
/// trying to setup a new connection or resume an existing one.
///
/// An instance of this class must be tied to a specific thread, as the
/// SetupResumeAcceptor::accept() entry point is not thread-safe.
class SetupResumeAcceptor final {
 public:
  using OnSetup = folly::Function<
      void(std::unique_ptr<DuplexConnection>, SetupParameters) noexcept>;
  using OnResume = folly::Function<
      void(std::unique_ptr<DuplexConnection>, ResumeParameters) noexcept>;

  explicit SetupResumeAcceptor(folly::EventBase*);
  ~SetupResumeAcceptor();

  /// Wait for and process the first frame on a DuplexConnection, calling the
  /// appropriate callback when the frame is received.  Not thread-safe.
  void accept(std::unique_ptr<DuplexConnection>, OnSetup, OnResume);

  /// Close all open connections, and prevent new ones from being accepted.  Can
  /// be called from any thread, and also after the EventBase has been
  /// destroyed, provided we know the ID of the owner thread.
  folly::Future<folly::Unit> close();

 private:
  class OneFrameSubscriber;

  void processFrame(
      std::unique_ptr<DuplexConnection>,
      std::unique_ptr<folly::IOBuf>,
      OnSetup,
      OnResume);

  /// Remove a OneFrameSubscriber from the set.
  void remove(const std::shared_ptr<OneFrameSubscriber>&);

  /// Close all open connections.
  void closeAll();

  /// Whether we're running in the thread that owns this object.  If the ctor
  /// specified an owner thread ID, then this will not access the EventBase
  /// pointer.
  ///
  /// Useful if the EventBase has been destroyed but we still want to do some
  /// work within the owner thread.
  bool inOwnerThread() const;

  std::unordered_set<std::shared_ptr<OneFrameSubscriber>> connections_;

  bool closed_{false};

  folly::EventBase* eventBase_;
};

} // namespace rsocket
