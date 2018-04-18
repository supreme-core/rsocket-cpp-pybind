// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncTransport.h>

#include "rsocket/ConnectionFactory.h"
#include "rsocket/DuplexConnection.h"

namespace folly {

class SSLContext;
}

namespace rsocket {

class RSocketStats;

/**
 * TCP implementation of ConnectionFactory for use with RSocket::createClient().
 *
 * Creation of this does nothing.  The `start` method kicks off work.
 */
class TcpConnectionFactory : public ConnectionFactory {
 public:
  TcpConnectionFactory(
      folly::EventBase& eventBase,
      folly::SocketAddress address,
      std::shared_ptr<folly::SSLContext> sslContext = nullptr);
  virtual ~TcpConnectionFactory();

  /**
   * Connect to server defined in constructor.
   *
   * Each call to connect() creates a new AsyncSocket.
   */
  folly::Future<ConnectedDuplexConnection> connect() override;

  static std::unique_ptr<DuplexConnection> createDuplexConnectionFromSocket(
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::shared_ptr<RSocketStats> stats = std::shared_ptr<RSocketStats>());

 private:
  folly::EventBase* eventBase_;
  const folly::SocketAddress address_;
  std::shared_ptr<folly::SSLContext> sslContext_;
};
} // namespace rsocket
