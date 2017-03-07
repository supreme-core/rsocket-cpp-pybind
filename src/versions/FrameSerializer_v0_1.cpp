// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v0_1.h"

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializerV0_1::Version;

ProtocolVersion FrameSerializerV0_1::protocolVersion() {
  return Version;
}

} // reactivesocket
