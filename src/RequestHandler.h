// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/Common.h"
#include "src/ConnectionSetupPayload.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"

namespace reactivesocket {

class StreamState;

class RequestHandler {
 public:
  virtual ~RequestHandler() = default;

  /// Handles a new Channel requested by the other end.
  virtual std::shared_ptr<Subscriber<Payload>> handleRequestChannel(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) = 0;

  /// Handles a new Stream requested by the other end.
  virtual void handleRequestStream(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) = 0;

  /// Handles a new inbound Subscription requested by the other end.
  virtual void handleRequestSubscription(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) = 0;

  /// Handles a new inbound RequestResponse requested by the other end.
  virtual void handleRequestResponse(
      Payload request,
      StreamId streamId,
      const std::shared_ptr<Subscriber<Payload>>& response) = 0;

  /// Handles a new fire-and-forget request sent by the other end.
  virtual void handleFireAndForgetRequest(
      Payload request,
      StreamId streamId) = 0;

  /// Handles a new metadata-push sent by the other end.
  virtual void handleMetadataPush(std::unique_ptr<folly::IOBuf> request) = 0;

  /// Temporary home - this should eventually be an input to asking for a
  /// RequestHandler so negotiation is possible
  virtual std::shared_ptr<StreamState> handleSetupPayload(
      ConnectionSetupPayload request) = 0;

  /// Temporary home - this should accompany handleSetupPayload
  /// Return stream state for the given token. Return nullptr to disable resume
  virtual std::shared_ptr<StreamState> handleResume(
      const ResumeIdentificationToken& token,
      ResumePosition position) = 0;

  // Handle a stream that can resume in a "clean" state. Client and Server are
  // up-to-date.
  virtual void handleCleanResume(std::shared_ptr<Subscription> response) = 0;

  // Handle a stream that can resume in a "dirty" state. Client is "behind"
  // Server.
  virtual void handleDirtyResume(std::shared_ptr<Subscription> response) = 0;
};
}
