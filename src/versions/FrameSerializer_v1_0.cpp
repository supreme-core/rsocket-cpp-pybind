// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v1_0.h"

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializerV1_0::Version;

ProtocolVersion FrameSerializerV1_0::protocolVersion() {
  return Version;
}

} // reactivesocket
