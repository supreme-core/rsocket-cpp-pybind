// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/ColdResumeHandler.h"

#include "yarpl/flowable/CancelingSubscriber.h"

#include <folly/Conv.h>

using namespace yarpl;
using namespace yarpl::flowable;

namespace rsocket {

std::string ColdResumeHandler::generateStreamToken(
    const Payload&,
    StreamId streamId,
    StreamType) {
  return folly::to<std::string>(streamId);
}

std::shared_ptr<Flowable<Payload>> ColdResumeHandler::handleResponderResumeStream(
    std::string /* streamToken */,
    size_t /* publisherAllowance */) {
  return Flowable<Payload>::error(
      std::logic_error("ResumeHandler method not implemented"));
}

std::shared_ptr<Subscriber<Payload>> ColdResumeHandler::handleRequesterResumeStream(
    std::string /* streamToken */,
    size_t /* consumerAllowance */) {
  return std::make_shared<CancelingSubscriber<Payload>>();
}
}
