// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/FrameSerializer.h"
#include <folly/Conv.h>
#include <folly/portability/GFlags.h>
#include "src/versions/FrameSerializer_v0.h"
#include "src/versions/FrameSerializer_v0_1.h"
#include "src/versions/FrameSerializer_v1_0.h"

DEFINE_string(
    rs_latest_protocol_version,
    "",
    "override for the latest ReactiveSocket protocol version to be used by "
    "the client [MAJOR.MINOR]");

namespace reactivesocket {

constexpr const ProtocolVersion ProtocolVersion::Unknown = ProtocolVersion(
    std::numeric_limits<uint16_t>::max(),
    std::numeric_limits<uint16_t>::max());

// TODO: this should default to 1.0 when we deploy successfully
constexpr static const ProtocolVersion kLatestProtocolVersion =
    FrameSerializerV0_1::Version;

ProtocolVersion FrameSerializer::getCurrentProtocolVersion() {
  if (FLAGS_rs_latest_protocol_version.empty()) {
    return kLatestProtocolVersion;
  }

  if (FLAGS_rs_latest_protocol_version.size() != 3) {
    LOG(ERROR) << "unknown protocol version "
               << FLAGS_rs_latest_protocol_version << " defaulting to v"
               << kLatestProtocolVersion;
    return kLatestProtocolVersion;
  }

  return ProtocolVersion(
      folly::to<uint16_t>(FLAGS_rs_latest_protocol_version[0]),
      folly::to<uint16_t>(FLAGS_rs_latest_protocol_version[2]));
}

std::unique_ptr<FrameSerializer> FrameSerializer::createFrameSerializer(
    const ProtocolVersion& protocolVersion) {
  if (protocolVersion == FrameSerializerV0::Version) {
    return std::make_unique<FrameSerializerV0>();
  } else if (protocolVersion == FrameSerializerV0_1::Version) {
    return std::make_unique<FrameSerializerV0_1>();
  } else if (protocolVersion == FrameSerializerV1_0::Version) {
    return std::make_unique<FrameSerializerV1_0>();
  }

  LOG(ERROR) << "unknown protocol version " << protocolVersion;
  return nullptr;
}

std::unique_ptr<FrameSerializer> FrameSerializer::createCurrentVersion() {
  return createFrameSerializer(getCurrentProtocolVersion());
}

std::ostream& operator<<(std::ostream& os, const ProtocolVersion& version) {
  return os << version.major << "." << version.minor;
}

} // reactivesocket
