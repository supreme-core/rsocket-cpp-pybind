// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <iosfwd>

#include <reactive-streams/utilities/AllowanceSemaphore.h>
#include <reactive-streams/utilities/SmartPointers.h>
#include "src/AbstractStreamAutomaton.h"
#include "src/Frame.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/mixins/ConsumerMixin.h"
#include "src/mixins/ExecutorMixin.h"
#include "src/mixins/LoggingMixin.h"
#include "src/mixins/MemoryMixin.h"
#include "src/mixins/MixinTerminator.h"
#include "src/mixins/PublisherMixin.h"
#include "src/mixins/SinkIfMixin.h"
#include "src/mixins/SourceIfMixin.h"
#include "src/mixins/StreamIfMixin.h"

namespace folly {
class exception_wrapper;
}

namespace reactivesocket {

enum class StreamCompletionSignal;

/// Implementation of stream automaton that represents a Channel requester.
class ChannelRequesterBase
    : public StreamIfMixin<PublisherMixin<
          Frame_REQUEST_CHANNEL,
          ConsumerMixin<Frame_RESPONSE, MixinTerminator>>>,
      public SubscriberBase,
      public SubscriptionBase {
  using Base = StreamIfMixin<PublisherMixin<
      Frame_REQUEST_CHANNEL,
      ConsumerMixin<Frame_RESPONSE, MixinTerminator>>>;

 public:
  struct Parameters : Base::Parameters {
    Parameters(const Base::Parameters& baseParams, folly::Executor& _executor)
        : Base::Parameters(baseParams), executor(_executor) {}
    folly::Executor& executor;
  };

  ChannelRequesterBase(const Parameters& params)
      : ExecutorBase(params.executor, false), Base(params) {}

 private:
  /// @{
  void onSubscribeImpl(std::shared_ptr<Subscription>) override;
  void onNextImpl(Payload) override;
  void onCompleteImpl() override;
  void onErrorImpl(folly::exception_wrapper) override;
  /// @}

  /// @{
  void requestImpl(size_t) override;
  void cancelImpl() override;
  /// @}

  /// @{
  void endStream(StreamCompletionSignal) override;

  /// Not all frames are intercepted, some just pass through.
  using Base::onNextFrame;
  void onNextFrame(Frame_RESPONSE&&) override;
  void onNextFrame(Frame_ERROR&&) override;
  /// @}

  /// State of the Channel requester.
  enum class State : uint8_t {
    NEW,
    REQUESTED,
    CLOSED,
  } state_{State::NEW};
  /// An allowance accumulated before the stream is initialised.
  /// Remaining part of the allowance is forwarded to the ConsumerMixin.
  reactivestreams::AllowanceSemaphore initialResponseAllowance_;
};

// using ChannelRequester = SourceIfMixin<SinkIfMixin<StreamIfMixin<
//     ExecutorMixin<MemoryMixin<LoggingMixin<ChannelRequesterBase>>>>>>;

using ChannelRequester = ChannelRequesterBase;
}
