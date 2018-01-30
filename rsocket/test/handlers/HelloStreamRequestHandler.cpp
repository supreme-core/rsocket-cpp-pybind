// Copyright 2004-present Facebook. All Rights Reserved.

#include "HelloStreamRequestHandler.h"
#include <folly/Conv.h>
#include <sstream>
#include "yarpl/Flowable.h"

using namespace ::rsocket;
using namespace yarpl;
using namespace yarpl::flowable;

namespace rsocket {
namespace tests {
/// Handles a new inbound Stream requested by the other end.
std::shared_ptr<Flowable<rsocket::Payload>>
HelloStreamRequestHandler::handleRequestStream(
    rsocket::Payload request,
    rsocket::StreamId) {
  VLOG(3) << "HelloStreamRequestHandler.handleRequestStream " << request;

  // string from payload data
  auto requestString = request.moveDataToString();

  return Flowable<>::range(1, 10)->map([name = std::move(requestString)](
      int64_t v) { return Payload(folly::to<std::string>(v), "metadata"); });
}
}
}
