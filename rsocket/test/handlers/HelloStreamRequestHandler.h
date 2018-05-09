// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/RSocketResponder.h"
#include "yarpl/Flowable.h"

namespace rsocket {
namespace tests {

class HelloStreamRequestHandler : public RSocketResponder {
 public:
  /// Handles a new inbound Stream requested by the other end.
  std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>
  handleRequestStream(rsocket::Payload request, rsocket::StreamId streamId)
      override;
};
} // namespace tests
} // namespace rsocket
