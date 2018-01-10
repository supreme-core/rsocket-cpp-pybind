// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/internal/Common.h"
#include "yarpl/flowable/Subscriber.h"
#include "yarpl/flowable/Subscription.h"
#include "yarpl/single/SingleObserver.h"

namespace folly {
class Executor;
}

namespace rsocket {

class RSocketStateMachine;
class ChannelResponder;
struct Payload;

class StreamsFactory {
 public:
  StreamsFactory(RSocketStateMachine& connection, RSocketMode mode);

  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> createChannelRequester(
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink);

  void createStreamRequester(
      Payload request,
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink);

  void createStreamRequester(
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> responseSink,
      StreamId streamId,
      size_t n);

  void createRequestResponseRequester(
      Payload payload,
      std::shared_ptr<yarpl::single::SingleObserver<Payload>> responseSink);

  // TODO: the return type should not be the stateMachine type, but something
  // generic
  std::shared_ptr<ChannelResponder> createChannelResponder(
      uint32_t initialRequestN,
      StreamId streamId);

  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> createStreamResponder(
      uint32_t initialRequestN,
      StreamId streamId);

  std::shared_ptr<yarpl::single::SingleObserver<Payload>>
  createRequestResponseResponder(StreamId streamId);

  bool registerNewPeerStreamId(StreamId streamId);
  StreamId getNextStreamId();

  void setNextStreamId(StreamId streamId);

 private:
  RSocketStateMachine& connection_;
  StreamId nextStreamId_;
  StreamId lastPeerStreamId_{0};
};
} // reactivesocket
