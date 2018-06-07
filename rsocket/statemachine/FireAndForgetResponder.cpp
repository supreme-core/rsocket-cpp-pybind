// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/FireAndForgetResponder.h"

namespace rsocket {

using namespace yarpl;
using namespace yarpl::flowable;

void FireAndForgetResponder::handlePayload(
    Payload&& payload,
    bool /*flagsComplete*/,
    bool /*flagsNext*/,
    bool flagsFollows) {
  payloadFragments_.addPayloadIgnoreFlags(std::move(payload));

  if (flagsFollows) {
    // there will be more fragments to come
    return;
  }

  Payload finalPayload = payloadFragments_.consumePayloadIgnoreFlags();
  onNewStreamReady(
      StreamType::FNF,
      std::move(finalPayload),
      std::shared_ptr<Subscriber<Payload>>(nullptr));
  removeFromWriter();
}

void FireAndForgetResponder::handleCancel() {
  removeFromWriter();
}

} // namespace rsocket
