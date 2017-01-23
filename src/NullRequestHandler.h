// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/ConnectionSetupPayload.h"
#include "src/RequestHandler.h"

namespace reactivesocket {

class NullSubscriber : public Subscriber<Payload> {
 public:
  // Subscriber methods
  void onSubscribe(std::shared_ptr<Subscription> subscription) override;
  void onNext(Payload element) override;
  void onComplete() override;
  void onError(folly::exception_wrapper ex) override;
};

class NullSubscription : public Subscription {
 public:
  // Subscription methods
  void request(size_t n) override;
  void cancel() override;
};

class NullRequestHandler : public RequestHandler {
 public:
  std::shared_ptr<Subscriber<Payload>> handleRequestChannel(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) override;

  void handleRequestStream(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) override;

  void handleRequestSubscription(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) override;

  void handleRequestResponse(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) override;

  void handleFireAndForgetRequest(Payload request, StreamId streamId) override;

  void handleMetadataPush(std::unique_ptr<folly::IOBuf> request) override;

  std::shared_ptr<StreamState> handleSetupPayload(
      ReactiveSocket& socket,
      ConnectionSetupPayload request) override;

  bool handleResume(
      ReactiveSocket& socket,
      const ResumeIdentificationToken& token,
      ResumePosition position) override;

  void handleCleanResume(
      std::shared_ptr<Subscription> response) noexcept override;
  void handleDirtyResume(
      std::shared_ptr<Subscription> response) noexcept override;

  void onSubscriptionPaused(
      const std::shared_ptr<Subscription>& subscription) noexcept override;
  void onSubscriptionResumed(
      const std::shared_ptr<Subscription>& subscription) noexcept override;
  void onSubscriberPaused(
      const std::shared_ptr<Subscriber<Payload>>& subscriber) noexcept override;
  void onSubscriberResumed(
      const std::shared_ptr<Subscriber<Payload>>& subscriber) noexcept override;
};

using DefaultRequestHandler = NullRequestHandler;
}
