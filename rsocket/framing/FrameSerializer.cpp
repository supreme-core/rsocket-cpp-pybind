// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/framing/FrameSerializer.h"

#include <folly/Conv.h>

#include "rsocket/framing/FrameSerializer_v0.h"
#include "rsocket/framing/FrameSerializer_v0_1.h"
#include "rsocket/framing/FrameSerializer_v1_0.h"

namespace rsocket {

constexpr const ProtocolVersion ProtocolVersion::Latest =
    FrameSerializerV1_0::Version;

std::unique_ptr<FrameSerializer> FrameSerializer::createFrameSerializer(
    const ProtocolVersion& protocolVersion) {
  if (protocolVersion == FrameSerializerV0::Version) {
    return std::make_unique<FrameSerializerV0>();
  } else if (protocolVersion == FrameSerializerV0_1::Version) {
    return std::make_unique<FrameSerializerV0_1>();
  } else if (protocolVersion == FrameSerializerV1_0::Version) {
    return std::make_unique<FrameSerializerV1_0>();
  }

  DCHECK(protocolVersion == ProtocolVersion::Unknown);
  LOG_IF(ERROR, protocolVersion != ProtocolVersion::Unknown)
      << "unknown protocol version " << protocolVersion;
  return nullptr;
}

std::unique_ptr<FrameSerializer> FrameSerializer::createAutodetectedSerializer(
    const folly::IOBuf& firstFrame) {
  auto detectedVersion = FrameSerializerV1_0::detectProtocolVersion(firstFrame);
  if (detectedVersion == ProtocolVersion::Unknown) {
    detectedVersion = FrameSerializerV0_1::detectProtocolVersion(firstFrame);
  }
  return createFrameSerializer(detectedVersion);
}

bool& FrameSerializer::preallocateFrameSizeField() {
  return preallocateFrameSizeField_;
}

folly::IOBufQueue FrameSerializer::createBufferQueue(size_t bufferSize) const {
  const auto prependSize =
      preallocateFrameSizeField_ ? frameLengthFieldSize() : 0;
  auto buf = folly::IOBuf::createCombined(bufferSize + prependSize);
  buf->advance(prependSize);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  queue.append(std::move(buf));
  return queue;
}


std::ostream& operator<<(std::ostream& os, const ProtocolVersion& version) {
  return os << version.major << "." << version.minor;
}
}
