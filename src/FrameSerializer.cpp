// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/FrameSerializer.h"
#include "src/versions/FrameSerializer_v0.h"
#include "src/versions/FrameSerializer_v0_1.h"
#include "src/versions/FrameSerializer_v1_0.h"

namespace reactivesocket {

std::unique_ptr<FrameSerializer> FrameSerializer::createFrameSerializer(
    std::string protocolVersion) {
  if (protocolVersion == "0.0") {
    return std::make_unique<FrameSerializerV0>();
  } else if (protocolVersion == "0.1") {
    return std::make_unique<FrameSerializerV0_1>();
  } else if (protocolVersion == "1.0") {
    return std::make_unique<FrameSerializerV1_0>();
  }

  // TODO: document current versions and compatibility with Java versions

  // TODO: we should just terminate cleanly without trying to read or write
  // any frames
  LOG(ERROR) << "unknown protocol version " << protocolVersion
             << " defaulting to v0.1";
  return std::make_unique<FrameSerializerV0_1>();
}

std::unique_ptr<FrameSerializer> FrameSerializer::createCurrentVersion() {
  return createFrameSerializer(kCurrentProtocolVersion);
}

} // reactivesocket
