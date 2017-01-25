// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/ConnectionSetupPayload.h"
#include "src/RequestHandler.h"

namespace reactivesocket {

class NullSubscriber : public Subscriber<Payload> {
 public:
  // Subscriber methods
  void onSubscribe(
      std::shared_ptr<Subscription> subscription) noexcept override;
  void onNext(Payload element) noexcept override;
  void onComplete() noexcept override;
  void onError(folly::exception_wrapper ex) noexcept override;
};

class NullSubscription : public Subscription {
 public:
  // Subscription methods
  void request(size_t n) noexcept override;
  void cancel() noexcept override;
};

class NullRequestHandler : public RequestHandler {
 public:
  std::shared_ptr<Subscriber<Payload>> handleRequestChannel(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override;

  void handleRequestStream(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override;

  void handleRequestSubscription(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override;

  void handleRequestResponse(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) noexcept override;

  void handleFireAndForgetRequest(
      Payload request,
      StreamId streamId) noexcept override;

  void handleMetadataPush(
      std::unique_ptr<folly::IOBuf> request) noexcept override;

  std::shared_ptr<StreamState> handleSetupPayload(
      ReactiveSocket& socket,
      ConnectionSetupPayload request) noexcept override;

  bool handleResume(
      ReactiveSocket& socket,
      const ResumeIdentificationToken& token,
      ResumePosition position) noexcept override;

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
