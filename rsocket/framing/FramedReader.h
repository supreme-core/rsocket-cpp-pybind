// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/IOBufQueue.h>

#include "rsocket/DuplexConnection.h"
#include "rsocket/framing/ProtocolVersion.h"
#include "rsocket/internal/Allowance.h"
#include "yarpl/flowable/Subscription.h"

namespace rsocket {

class FramedReader : public DuplexConnection::DuplexSubscriber,
                     public yarpl::flowable::Subscription,
                     public yarpl::enable_get_ref {
 public:
  explicit FramedReader(std::shared_ptr<ProtocolVersion> version)
      : version_{std::move(version)} {}

  /// Set the inner subscriber which will be getting full frame payloads.
  void setInput(std::shared_ptr<DuplexConnection::Subscriber>);

  /// Cancel the subscription and error the inner subscriber.
  void error(std::string);

  // Subscriber.

  void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>) override;
  void onNext(std::unique_ptr<folly::IOBuf>) override;
  void onComplete() override;
  void onError(folly::exception_wrapper) override;

  // Subscription.

  void request(int64_t) override;
  void cancel() override;

 private:
  void parseFrames();
  bool ensureOrAutodetectProtocolVersion();

  size_t readFrameLength() const;

  std::shared_ptr<DuplexConnection::Subscriber> inner_;

  Allowance allowance_;
  bool dispatchingFrames_{false};

  folly::IOBufQueue payloadQueue_{folly::IOBufQueue::cacheChainLength()};
  const std::shared_ptr<ProtocolVersion> version_;
};
} // namespace rsocket
