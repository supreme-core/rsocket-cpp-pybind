// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include "rsocket/DuplexConnection.h"
#include "rsocket/internal/Common.h"
#include "yarpl/flowable/Subscription.h"

#include "rsocket/framing/FrameTransport.h"

namespace rsocket {

class FrameProcessor;

class FrameTransportImpl
    : public FrameTransport,
      /// Registered as an input in the DuplexConnection.
      public DuplexConnection::Subscriber,
      public std::enable_shared_from_this<FrameTransportImpl> {
 public:
  explicit FrameTransportImpl(std::unique_ptr<DuplexConnection> connection);
  ~FrameTransportImpl();

  void setFrameProcessor(std::shared_ptr<FrameProcessor>) override;

  /// Writes the frame directly to output. If the connection was closed it will
  /// drop the frame.
  void outputFrameOrDrop(std::unique_ptr<folly::IOBuf>) override;

  /// Cancel the input and close the underlying connection.
  void close() override;

  bool isClosed() const {
    return !connection_;
  }

  DuplexConnection* getConnection() override {
    return connection_.get();
  }

  bool isConnectionFramed() const override;

  // Subscriber.

  void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>) override;
  void onNext(std::unique_ptr<folly::IOBuf>) override;
  void onComplete() override;
  void onError(folly::exception_wrapper) override;

 private:
  void connect();

  /// Terminates the FrameProcessor.  Will queue up the exception if no
  /// processor is set, overwriting any previously queued exception.
  void terminateProcessor(folly::exception_wrapper);

  std::shared_ptr<FrameProcessor> frameProcessor_;
  std::shared_ptr<DuplexConnection> connection_;

  std::shared_ptr<DuplexConnection::Subscriber> connectionOutput_;
  std::shared_ptr<yarpl::flowable::Subscription> connectionInputSub_;
};

} // namespace rsocket
