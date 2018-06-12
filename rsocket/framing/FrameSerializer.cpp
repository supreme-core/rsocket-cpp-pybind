// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/framing/FrameSerializer.h"

#include "rsocket/framing/FrameSerializer_v1_0.h"

namespace rsocket {

std::unique_ptr<FrameSerializer> FrameSerializer::createFrameSerializer(
    const ProtocolVersion& protocolVersion) {
  if (protocolVersion == FrameSerializerV1_0::Version) {
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

} // namespace rsocket
