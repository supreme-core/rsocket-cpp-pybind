// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/statemachine/ConsumerBase.h"

namespace rsocket {

/// Implementation of stream stateMachine that represents a Stream requester
class StreamRequester : public ConsumerBase {
 public:
  StreamRequester(
      std::shared_ptr<StreamsWriter> writer,
      StreamId streamId,
      Payload payload)
      : ConsumerBase(std::move(writer), streamId),
        initialPayload_(std::move(payload)) {}

  void setRequested(size_t);

  void request(int64_t) override;
  void cancel() override;

  void handlePayload(
      Payload&& payload,
      bool flagsComplete,
      bool flagsNext,
      bool flagsFollows) override;
  void handleError(folly::exception_wrapper errorPayload) override;

 private:
  /// Payload to be sent with the first request.
  Payload initialPayload_;

  /// Whether request() has been called.
  bool requested_{false};
};

} // namespace rsocket
