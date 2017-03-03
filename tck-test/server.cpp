// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/Memory.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gmock/gmock.h>

#include "src/NullRequestHandler.h"
#include "src/StandardReactiveSocket.h"
#include "src/SubscriptionBase.h"
#include "src/framed/FramedDuplexConnection.h"
#include "src/tcp/TcpDuplexConnection.h"

#include "test/simple/StatsPrinter.h"

using namespace ::testing;
using namespace ::reactivesocket;
using namespace ::folly;

DEFINE_string(address, "9898", "port to listen to");

namespace {
class ServerSubscription : public SubscriptionBase {
 public:
  explicit ServerSubscription(
      std::shared_ptr<Subscriber<Payload>> response,
      size_t numElems = 2)
      : ExecutorBase(defaultExecutor()),
        response_(std::move(response)),
        numElems_(numElems) {}

 private:
  void requestImpl(size_t n) noexcept override {
    for (size_t i = 0; i < numElems_; i++) {
      response_->onNext(Payload("from server " + std::to_string(i)));
    }
    response_->onComplete();
  }

  void cancelImpl() noexcept override {}

  std::shared_ptr<Subscriber<Payload>> response_;
  size_t numElems_;
};

class ServerRequestHandler : public DefaultRequestHandler {
 public:
  /// Handles a new inbound Subscription requested by the other end.
  void handleRequestSubscription(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleRequestSubscription " << request;

    response->onSubscribe(std::make_shared<ServerSubscription>(response));
  }

  /// Handles a new inbound Stream requested by the other end.
  void handleRequestStream(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleRequestStream " << request;

    response->onSubscribe(std::make_shared<ServerSubscription>(response));
  }

  void handleRequestResponse(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleRequestResponse " << request;

    response->onSubscribe(std::make_shared<ServerSubscription>(response, 1));
  }

  void handleFireAndForgetRequest(
      Payload request,
      StreamId streamId) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleFireAndForgetRequest " << request;
  }

  void handleMetadataPush(
      std::unique_ptr<folly::IOBuf> request) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleMetadataPush "
              << request->moveToFbString();
  }

  std::shared_ptr<StreamState> handleSetupPayload(
      ReactiveSocket&,
      ConnectionSetupPayload request) noexcept override {
    LOG(INFO) << "ServerRequestHandler.handleSetupPayload " << request;
    return nullptr;
  }

};

class Callback : public AsyncServerSocket::AcceptCallback {
 public:
  Callback(EventBase& eventBase, Stats& stats)
      : eventBase_(eventBase), stats_(stats){};

  virtual ~Callback() = default;

  virtual void connectionAccepted(
      int fd,
      const SocketAddress& clientAddr) noexcept override {
    LOG(INFO) << "ConnectionAccepted " << clientAddr.describe() << std::endl;

    auto socket =
        folly::AsyncSocket::UniquePtr(new AsyncSocket(&eventBase_, fd));

    std::unique_ptr<DuplexConnection> connection =
        std::make_unique<TcpDuplexConnection>(
            std::move(socket), inlineExecutor(), stats_);
    std::unique_ptr<DuplexConnection> framedConnection =
        std::make_unique<FramedDuplexConnection>(
            std::move(connection), inlineExecutor());
    std::unique_ptr<RequestHandler> requestHandler =
        std::make_unique<ServerRequestHandler>();

    auto rs = StandardReactiveSocket::fromServerConnection(
        eventBase_,
        std::move(framedConnection),
        std::move(requestHandler),
        stats_);

    rs->onClosed([ this, rs = rs.get() ](const folly::exception_wrapper& ex) {
      removeSocket(*rs);
    });

    reactiveSockets_.push_back(std::move(rs));
  }

  void removeSocket(ReactiveSocket& socket) {
    if (!shuttingDown) {
      reactiveSockets_.erase(std::remove_if(
          reactiveSockets_.begin(),
          reactiveSockets_.end(),
          [&socket](std::unique_ptr<StandardReactiveSocket>& vecSocket) {
            return vecSocket.get() == &socket;
          }));
    }
  }

  virtual void acceptError(const std::exception& ex) noexcept override {
    std::cout << "AcceptError " << ex.what() << std::endl;
  }

  void shutdown() {
    shuttingDown = true;
    reactiveSockets_.clear();
  }

 private:
  std::vector<std::unique_ptr<StandardReactiveSocket>> reactiveSockets_;
  EventBase& eventBase_;
  Stats& stats_;
  bool shuttingDown{false};
};
}

void signal_handler(int signal) {
  LOG(INFO) << "Terminating program after receiving signal " << signal;
  exit(signal);
}

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  folly::ScopedEventBaseThread evbt;
  reactivesocket::StatsPrinter statsPrinter;
  Callback callback(*evbt.getEventBase(), statsPrinter);

  auto serverSocket = AsyncServerSocket::newSocket(evbt.getEventBase());

  evbt.getEventBase()->runInEventBaseThreadAndWait(
      [&evbt, &serverSocket, &callback]() {
        folly::SocketAddress addr;
        addr.setFromLocalIpPort(FLAGS_address);
        serverSocket->setReusePortEnabled(true);
        serverSocket->bind(addr);
        serverSocket->addAcceptCallback(&callback, evbt.getEventBase());
        serverSocket->listen(10);
        serverSocket->startAccepting();
        LOG(INFO) << "Server listening on "
                  << serverSocket->getAddress().describe();
      });



  while (true)
    ;
}
