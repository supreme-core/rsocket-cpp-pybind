// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <atomic>
#include <list>
#include <memory>

#include "src/Common.h"
#include "src/ConnectionSetupPayload.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/Stats.h"

namespace folly {
class Executor;
}

namespace reactivesocket {

class ClientResumeStatusCallback;
class ConnectionAutomaton;
class DuplexConnection;
class FrameTransport;
class RequestHandler;
class ReactiveSocket;

folly::Executor& defaultExecutor();

// TODO(stupaq): Here is some heavy problem with the recursion on shutdown.
// Giving someone ownership over this object would probably lead to a deadlock
// (or a crash) if one calls ::~ReactiveSocket from a terminal signal handler
// of any of the connections. It seems natural to follow ReactiveStreams
// specification and make this object "self owned", so that we don't need to
// perform all of the cleanup from a d'tor. We could give out a "proxy" object
// that (internally) holds a weak reference (conceptually, not std::weak_ptr)
// and forbids any interactions with the socket after the shutdown procedure has
// been initiated.
class ReactiveSocket {
 public:
  ReactiveSocket(ReactiveSocket&&) = delete;
  ReactiveSocket& operator=(ReactiveSocket&&) = delete;
  ReactiveSocket(const ReactiveSocket&) = delete;
  ReactiveSocket& operator=(const ReactiveSocket&) = delete;

  ~ReactiveSocket();

  static std::unique_ptr<ReactiveSocket> fromClientConnection(
      folly::Executor& executor,
      std::unique_ptr<DuplexConnection> connection,
      std::unique_ptr<RequestHandler> handler,
      ConnectionSetupPayload setupPayload = ConnectionSetupPayload(),
      Stats& stats = Stats::noop(),
      std::unique_ptr<KeepaliveTimer> keepaliveTimer =
          std::unique_ptr<KeepaliveTimer>(nullptr));

  static std::unique_ptr<ReactiveSocket> disconnectedClient(
      folly::Executor& executor,
      std::unique_ptr<RequestHandler> handler,
      Stats& stats = Stats::noop(),
      std::unique_ptr<KeepaliveTimer> keepaliveTimer =
          std::unique_ptr<KeepaliveTimer>(nullptr));

  static std::unique_ptr<ReactiveSocket> fromServerConnection(
      folly::Executor& executor,
      std::unique_ptr<DuplexConnection> connection,
      std::unique_ptr<RequestHandler> handler,
      Stats& stats = Stats::noop(),
      bool isResumable = false);

  static std::unique_ptr<ReactiveSocket> disconnectedServer(
      folly::Executor& executor,
      std::unique_ptr<RequestHandler> handler,
      Stats& stats = Stats::noop());

  std::shared_ptr<Subscriber<Payload>> requestChannel(
      const std::shared_ptr<Subscriber<Payload>>& responseSink);

  void requestStream(
      Payload payload,
      const std::shared_ptr<Subscriber<Payload>>& responseSink);

  void requestSubscription(
      Payload payload,
      const std::shared_ptr<Subscriber<Payload>>& responseSink);

  void requestResponse(
      Payload payload,
      const std::shared_ptr<Subscriber<Payload>>& responseSink);

  void requestFireAndForget(Payload request);

  void metadataPush(std::unique_ptr<folly::IOBuf> metadata);

  void clientConnect(
      std::shared_ptr<FrameTransport> frameTransport,
      ConnectionSetupPayload setupPayload = ConnectionSetupPayload());

  void serverConnect(
      std::shared_ptr<FrameTransport> frameTransport,
      bool isResumable);

  void close();
  void disconnect();
  std::shared_ptr<FrameTransport> detachFrameTransport();

  void onConnected(ReactiveSocketCallback listener);
  void onDisconnected(ReactiveSocketCallback listener);
  void onClosed(ReactiveSocketCallback listener);

  void tryClientResume(
      const ResumeIdentificationToken& token,
      std::shared_ptr<FrameTransport> frameTransport,
      std::unique_ptr<ClientResumeStatusCallback> resumeCallback);

  bool tryResumeServer(
      std::shared_ptr<FrameTransport> frameTransport,
      ResumePosition position);

  folly::Executor& executor() {
    return executor_;
  }

  DuplexConnection* duplexConnection() const;

 private:
  ReactiveSocket(
      bool isServer,
      std::shared_ptr<RequestHandler> handler,
      Stats& stats,
      std::unique_ptr<KeepaliveTimer> keepaliveTimer,
      folly::Executor& executor);

  void createResponder(
      std::shared_ptr<RequestHandler> handler,
      ConnectionAutomaton& connection,
      StreamId streamId,
      std::unique_ptr<folly::IOBuf> frame);

  std::shared_ptr<StreamState> resumeListener(
      const ResumeIdentificationToken& token);

  std::function<void()> executeListenersFunc(
      std::shared_ptr<std::list<ReactiveSocketCallback>> listeners);

  void checkNotClosed() const;

  std::shared_ptr<RequestHandler> handler_;

  std::shared_ptr<std::list<ReactiveSocketCallback>> onConnectListeners_{
      std::make_shared<std::list<ReactiveSocketCallback>>()};
  std::shared_ptr<std::list<ReactiveSocketCallback>> onDisconnectListeners_{
      std::make_shared<std::list<ReactiveSocketCallback>>()};
  std::shared_ptr<std::list<ReactiveSocketCallback>> onCloseListeners_{
      std::make_shared<std::list<ReactiveSocketCallback>>()};

  std::shared_ptr<ConnectionAutomaton> connection_;
  StreamId nextStreamId_;
  folly::Executor& executor_;
};
}
