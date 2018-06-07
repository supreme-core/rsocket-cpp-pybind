// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/Payload.h"
#include "rsocket/statemachine/StreamStateMachineBase.h"
#include "yarpl/single/SingleObserver.h"
#include "yarpl/single/SingleSubscription.h"

namespace rsocket {

/// Implementation of stream stateMachine that represents a RequestResponse
/// requester
class RequestResponseRequester
    : public StreamStateMachineBase,
      public yarpl::single::SingleSubscription,
      public std::enable_shared_from_this<RequestResponseRequester> {
 public:
  RequestResponseRequester(
      std::shared_ptr<StreamsWriter> writer,
      StreamId streamId,
      Payload payload)
      : StreamStateMachineBase(std::move(writer), streamId),
        initialPayload_(std::move(payload)) {}

  void subscribe(
      std::shared_ptr<yarpl::single::SingleObserver<Payload>> subscriber);

 private:
  void cancel() noexcept override;

  void handlePayload(
      Payload&& payload,
      bool flagsComplete,
      bool flagsNext,
      bool flagsFollows) override;
  void handleError(folly::exception_wrapper errorPayload) override;

  void endStream(StreamCompletionSignal signal) override;

  size_t getConsumerAllowance() const override;

  /// State of the Subscription requester.
  enum class State : uint8_t {
    NEW,
    REQUESTED,
    CLOSED,
  };

  State state_{State::NEW};

  /// The observer that will consume payloads.
  std::shared_ptr<yarpl::single::SingleObserver<Payload>> consumingSubscriber_;

  /// Initial payload which has to be sent with 1st request.
  Payload initialPayload_;
};
} // namespace rsocket
