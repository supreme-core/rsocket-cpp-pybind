// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include <cstddef>

#include "rsocket/Payload.h"
#include "rsocket/internal/Allowance.h"
#include "rsocket/internal/Common.h"
#include "rsocket/statemachine/RSocketStateMachine.h"
#include "rsocket/statemachine/StreamStateMachineBase.h"
#include "yarpl/flowable/Subscription.h"

namespace rsocket {

enum class StreamCompletionSignal;

/// A class that represents a flow-control-aware consumer of data.
class ConsumerBase : public StreamStateMachineBase,
                     public yarpl::flowable::Subscription,
                     public std::enable_shared_from_this<ConsumerBase> {
 public:
  using StreamStateMachineBase::StreamStateMachineBase;

  /// Adds implicit allowance.
  ///
  /// This portion of allowance will not be synced to the remote end, but will
  /// count towards the limit of allowance the remote PublisherBase may use.
  void addImplicitAllowance(size_t n);

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> subscriber);

  void generateRequest(size_t n);

  size_t getConsumerAllowance() const override;

  bool consumerClosed() const {
    return state_ == State::CLOSED;
  }

 protected:
  void cancelConsumer();

  void endStream(StreamCompletionSignal signal) override;

  void processPayload(Payload&&, bool onNext);

  void completeConsumer();
  void errorConsumer(folly::exception_wrapper ex);

 private:
  void sendRequests();

  void handleFlowControlError();

  /// A Subscriber that will consume payloads.
  /// This is responsible for delivering a terminal signal to the
  /// Subscriber once the stream ends.
  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> consumingSubscriber_;

  /// A total, net allowance (requested less delivered) by this consumer.
  Allowance allowance_;
  /// An allowance that have yet to be synced to the other end by sending
  /// REQUEST_N frames.
  Allowance pendingAllowance_;

  /// The number of already requested payload count.
  /// Prevent excessive requestN calls.
  Allowance activeRequests_;

  enum class State : uint8_t {
    RESPONDING,
    CLOSED,
  };

  State state_{State::RESPONDING};
};

} // namespace rsocket
