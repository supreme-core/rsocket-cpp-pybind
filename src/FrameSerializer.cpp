// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/FrameSerializer.h"
#include "src/versions/FrameSerializer_v0.h"
#include "src/versions/FrameSerializer_v0_1.h"
#include "src/versions/FrameSerializer_v1_0.h"

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializer::kCurrentProtocolVersion;

std::unique_ptr<FrameSerializer> FrameSerializer::createFrameSerializer(
    const ProtocolVersion& protocolVersion) {
  if (protocolVersion == FrameSerializerV0::Version) {
    return std::make_unique<FrameSerializerV0>();
  } else if (protocolVersion == FrameSerializerV0_1::Version) {
    return std::make_unique<FrameSerializerV0_1>();
  } else if (protocolVersion == FrameSerializerV1_0::Version) {
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

std::ostream& operator<<(std::ostream& os, const ProtocolVersion& version) {
  return os << version.major << "." << version.minor;
}

} // reactivesocket
