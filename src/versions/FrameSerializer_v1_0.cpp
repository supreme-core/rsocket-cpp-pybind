// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v1_0.h"
#include <folly/io/Cursor.h>

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializerV1_0::Version;

ProtocolVersion FrameSerializerV1_0::protocolVersion() {
  return Version;
}

static FrameType deserializeFrameType(uint16_t frameType) {
  if (frameType > static_cast<uint8_t>(FrameType::RESUME_OK) &&
      frameType != static_cast<uint8_t>(FrameType::EXT)) {
    return FrameType::RESERVED;
  }
  return static_cast<FrameType>(frameType);
}

FrameType FrameSerializerV1_0::peekFrameType(const folly::IOBuf& in) {
  folly::io::Cursor cur(&in);
  try {
    cur.skip(sizeof(int32_t)); // streamId
    uint8_t type = cur.readBE<uint8_t>(); // |Frame Type |I|M|
    return deserializeFrameType(type >> 2);
  } catch (...) {
    return FrameType::RESERVED;
  }
}

folly::Optional<StreamId> FrameSerializerV1_0::peekStreamId(
    const folly::IOBuf& in) {
  folly::io::Cursor cur(&in);
  try {
    auto streamId = cur.readBE<int32_t>();
    if (streamId < 0) {
      return folly::none;
    }
    return folly::make_optional(static_cast<StreamId>(streamId));
  } catch (...) {
    return folly::none;
  }
}

void FrameSerializerV1_0::serializeHeaderInto(
    folly::io::QueueAppender& appender,
    const FrameHeader& header) {
  appender.writeBE<int32_t>(header.streamId_);

  auto type = static_cast<uint8_t>(header.type_); // 6 bit
  auto flags = static_cast<uint16_t>(header.flags_); // 10 bit
  appender.writeBE<uint8_t>((type << 2) | (flags >> 8));
  appender.writeBE<uint8_t>(flags); // lower 8 bits
}

void FrameSerializerV1_0::deserializeHeaderFrom(
    folly::io::Cursor& cur,
    FrameHeader& header) {
  auto streamId = cur.readBE<int32_t>();
  if (streamId < 0) {
    throw std::runtime_error("invalid stream id");
  }
  header.streamId_ = streamId;
  uint16_t type = cur.readBE<uint8_t>(); // type + I,M flags
  header.type_ = deserializeFrameType(type >> 2);
  header.flags_ =
      static_cast<FrameFlags>(((type & 0x3) << 8) | cur.readBE<uint8_t>());
}

} // reactivesocket
