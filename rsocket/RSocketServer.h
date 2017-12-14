// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <mutex>

#include <folly/Synchronized.h>
#include <folly/ThreadLocal.h>
#include <folly/synchronization/Baton.h>

#include "rsocket/ConnectionAcceptor.h"
#include "rsocket/RSocketParameters.h"
#include "rsocket/RSocketResponder.h"
#include "rsocket/RSocketServiceHandler.h"
#include "rsocket/internal/SetupResumeAcceptor.h"

namespace rsocket {

/**
 * API for starting an RSocket server. Returned from RSocket::createServer.
 *
 * This listens for connections using a transport from the provided
 * ConnectionAcceptor.
 *
 */
class RSocketServer {
 public:
  explicit RSocketServer(
      std::unique_ptr<ConnectionAcceptor>,
      std::shared_ptr<RSocketStats> stats = RSocketStats::noop());
  ~RSocketServer();

  RSocketServer(const RSocketServer&) = delete;
  RSocketServer(RSocketServer&&) = delete;
  RSocketServer& operator=(const RSocketServer&) = delete;
  RSocketServer& operator=(RSocketServer&&) = delete;

  /**
   * Start the ConnectionAcceptor and begin handling connections.
   *
   * This method blocks until the server has started. It returns if successful
   * or throws an exception if failure occurs.
   *
   * This method assumes it will be called only once.
   */
  void start(std::shared_ptr<RSocketServiceHandler> serviceHandler);
  void start(OnNewSetupFn onNewSetupFn);

  /**
   * Start the ConnectionAcceptor and begin handling connections.
   *
   * This method will block the calling thread as long as the server is running.
   * It will throw an exception if a failure occurs on startup.
   *
   * The provided RSocketServiceHandler will be used to handle all connections
   * to this server. If you wish to use different RSocketServiceHandler for
   * each connection, then refer to acceptConnection()
   *
   * This method assumes it will be called only once.
   */
  void startAndPark(std::shared_ptr<RSocketServiceHandler> serviceHandler);
  void startAndPark(OnNewSetupFn onNewSetupFn);

  /**
   * Unblock the server if it has called startAndPark().  Can only be called
   * once.
   */
  void unpark();

  /**
   * Accept RSocket connection over the provided DuplexConnection.  The
   * provided RSocketServiceHandler will be used to handle the connection.
   */
  void acceptConnection(
      std::unique_ptr<DuplexConnection> connection,
      folly::EventBase& eventBase,
      std::shared_ptr<RSocketServiceHandler> serviceHandler);

  void shutdownAndWait();

  /**
   * Gets the port the ConnectionAcceptor is listening on.  Returns folly::none
   * if this server is not listening on a port.
   */
  folly::Optional<uint16_t> listeningPort() const;

  /**
   * Use the same EventBase that is provided to acceptConnection function for
   * internal operations. Don't schedule to another event base.
   */
  void setSingleThreadedResponder();

 private:
  void onRSocketSetup(
      std::shared_ptr<RSocketServiceHandler> serviceHandler,
      yarpl::Reference<rsocket::FrameTransport> frameTransport,
      rsocket::SetupParameters setupPayload);
  void onRSocketResume(
      std::shared_ptr<RSocketServiceHandler> serviceHandler,
      yarpl::Reference<rsocket::FrameTransport> frameTransport,
      rsocket::ResumeParameters setupPayload);

  std::unique_ptr<ConnectionAcceptor> duplexConnectionAcceptor_;
  bool started{false};

  class SetupResumeAcceptorTag {};
  folly::ThreadLocal<rsocket::SetupResumeAcceptor, SetupResumeAcceptorTag>
      setupResumeAcceptors_;

  folly::Baton<> waiting_;
  std::atomic<bool> isShutdown_{false};

  std::shared_ptr<ConnectionSet> connectionSet_;
  std::shared_ptr<RSocketStats> stats_;

  /**
   * If this field is false, acceptConnection() function will assume that there
   * will be a single thread for each connected client. The execution will not
   * be scheduled to another event base.
   */
  bool useScheduledResponder_{true};
};
} // namespace rsocket
