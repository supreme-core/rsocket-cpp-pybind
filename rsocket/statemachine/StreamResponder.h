// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/statemachine/PublisherBase.h"
#include "rsocket/statemachine/StreamStateMachineBase.h"
#include "yarpl/flowable/Subscriber.h"

namespace rsocket {

/// Implementation of stream stateMachine that represents a Stream responder
class StreamResponder : public StreamStateMachineBase,
                        public PublisherBase,
                        public yarpl::flowable::Subscriber<Payload>,
                        public std::enable_shared_from_this<StreamResponder> {
 public:
  StreamResponder(
      std::shared_ptr<StreamsWriter> writer,
      StreamId streamId,
      uint32_t initialRequestN)
      : StreamStateMachineBase(std::move(writer), streamId),
        PublisherBase(initialRequestN) {}

  void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>) override;
  void onNext(Payload) override;
  void onComplete() override;
  void onError(folly::exception_wrapper) override;

  void handlePayload(
      Payload&& payload,
      bool flagsComplete,
      bool flagsNext,
      bool flagsFollows) override;
  void handleRequestN(uint32_t) override;
  void handleError(folly::exception_wrapper) override;
  void handleCancel() override;

  void endStream(StreamCompletionSignal) override;

 private:
  bool newStream_{true};
};

} // namespace rsocket
