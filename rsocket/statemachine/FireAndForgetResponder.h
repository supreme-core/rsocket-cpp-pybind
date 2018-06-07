// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/statemachine/StreamStateMachineBase.h"
#include "yarpl/flowable/Subscriber.h"
#include "yarpl/single/SingleObserver.h"
#include "yarpl/single/SingleSubscription.h"

namespace rsocket {

/// Helper class for handling receiving fragmented payload
class FireAndForgetResponder : public StreamStateMachineBase {
 public:
  FireAndForgetResponder(
      std::shared_ptr<StreamsWriter> writer,
      StreamId streamId)
      : StreamStateMachineBase(std::move(writer), streamId) {}

  void handlePayload(
      Payload&& payload,
      bool flagsComplete,
      bool flagsNext,
      bool flagsFollows) override;

 private:
  void handleCancel() override;
};
} // namespace rsocket
