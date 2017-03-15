// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v0.h"
#include <folly/io/Cursor.h>

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializerV0::Version;

enum class FrameFlags_V0 : uint16_t {
  EMPTY = 0x0000,
  IGNORE = 0x8000,
  METADATA = 0x4000,

  FOLLOWS = 0x2000,
  KEEPALIVE_RESPOND = 0x2000,
  LEASE = 0x2000,
  COMPLETE = 0x1000,
  STRICT = 0x1000,
  RESUME_ENABLE = 0x0800,
};

namespace {
enum class FrameType_V0 : uint16_t {
  RESERVED = 0x0000,
  SETUP = 0x0001,
  LEASE = 0x0002,
  KEEPALIVE = 0x0003,
  REQUEST_RESPONSE = 0x0004,
  REQUEST_FNF = 0x0005,
  REQUEST_STREAM = 0x0006,
  REQUEST_SUB = 0x0007,
  REQUEST_CHANNEL = 0x0008,
  REQUEST_N = 0x0009,
  CANCEL = 0x000A,
  RESPONSE = 0x000B,
  ERROR = 0x000C,
  METADATA_PUSH = 0x000D,
  RESUME = 0x000E,
  RESUME_OK = 0x000F,
  EXT = 0xFFFF,
};

constexpr inline FrameFlags_V0 operator&(FrameFlags_V0 a, FrameFlags_V0 b) {
  return static_cast<FrameFlags_V0>(
      static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline uint16_t& operator|=(uint16_t& a, FrameFlags_V0 b) {
  return (a |= static_cast<uint16_t>(b));
}

constexpr inline bool operator!(FrameFlags_V0 a) {
  return !static_cast<uint16_t>(a);
}
}

static folly::IOBufQueue createBufferQueue(size_t bufferSize) {
  auto buf = reactivesocket::FrameBufferAllocator::allocate(bufferSize);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  queue.append(std::move(buf));
  return queue;
}

ProtocolVersion FrameSerializerV0::protocolVersion() {
  return Version;
}

static uint16_t serializeFrameType(FrameType frameType) {
  switch (frameType) {
    case FrameType::RESERVED:
    case FrameType::SETUP:
    case FrameType::LEASE:
    case FrameType::KEEPALIVE:
    case FrameType::REQUEST_RESPONSE:
    case FrameType::REQUEST_FNF:
    case FrameType::REQUEST_STREAM:
      return static_cast<uint16_t>(frameType);

    case FrameType::REQUEST_CHANNEL:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::PAYLOAD:
    case FrameType::ERROR:
    case FrameType::METADATA_PUSH:
    case FrameType::RESUME:
    case FrameType::RESUME_OK:
      return static_cast<uint16_t>(frameType) + 1;

    case FrameType::EXT:
      return static_cast<uint16_t>(FrameType_V0::EXT);

    default:
      CHECK(false);
      return 0;
  }
}

static FrameType deserializeFrameType(uint16_t frameType) {
  if (frameType > static_cast<uint16_t>(FrameType_V0::RESUME_OK) &&
      frameType != static_cast<uint16_t>(FrameType_V0::EXT)) {
    return FrameType::RESERVED;
  }

  switch (static_cast<FrameType_V0>(frameType)) {
    case FrameType_V0::RESERVED:
    case FrameType_V0::SETUP:
    case FrameType_V0::LEASE:
    case FrameType_V0::KEEPALIVE:
    case FrameType_V0::REQUEST_RESPONSE:
    case FrameType_V0::REQUEST_FNF:
    case FrameType_V0::REQUEST_STREAM:
      return static_cast<FrameType>(frameType);

    case FrameType_V0::REQUEST_SUB:
      return FrameType::REQUEST_STREAM;

    case FrameType_V0::REQUEST_CHANNEL:
    case FrameType_V0::REQUEST_N:
    case FrameType_V0::CANCEL:
    case FrameType_V0::RESPONSE:
    case FrameType_V0::ERROR:
    case FrameType_V0::METADATA_PUSH:
    case FrameType_V0::RESUME:
    case FrameType_V0::RESUME_OK:
      return static_cast<FrameType>(frameType - 1);

    case FrameType_V0::EXT:
      return FrameType::EXT;

    default:
      CHECK(false);
      return FrameType::RESERVED;
  }
}

static uint16_t serializeFrameFlags(FrameFlags frameType) {
  uint16_t result = 0;
  if (!!(frameType & FrameFlags::IGNORE)) {
    result |= FrameFlags_V0::IGNORE;
  }
  if (!!(frameType & FrameFlags::METADATA)) {
    result |= FrameFlags_V0::METADATA;
  }
  return result;
}

static FrameFlags deserializeFrameFlags(FrameFlags_V0 flags) {
  FrameFlags result = FrameFlags::EMPTY;

  if (!!(flags & FrameFlags_V0::IGNORE)) {
    result |= FrameFlags::IGNORE;
  }
  if (!!(flags & FrameFlags_V0::METADATA)) {
    result |= FrameFlags::METADATA;
  }
  return result;
}

FrameType FrameSerializerV0::peekFrameType(const folly::IOBuf& in) {
  folly::io::Cursor cur(&in);
  try {
    return deserializeFrameType(cur.readBE<uint16_t>());
  } catch (...) {
    return FrameType::RESERVED;
  }
}

folly::Optional<StreamId> FrameSerializerV0::peekStreamId(
    const folly::IOBuf& in) {
  folly::io::Cursor cur(&in);
  try {
    cur.skip(sizeof(uint16_t)); // type
    cur.skip(sizeof(uint16_t)); // flags
    return folly::make_optional(cur.readBE<uint32_t>());
  } catch (...) {
    return folly::none;
  }
}

void FrameSerializerV0::serializeHeaderInto(
    folly::io::QueueAppender& appender,
    const FrameHeader& header,
    uint16_t extraFlags) {
  appender.writeBE<uint16_t>(serializeFrameType(header.type_));
  appender.writeBE<uint16_t>(serializeFrameFlags(header.flags_) | extraFlags);
  appender.writeBE<uint32_t>(header.streamId_);
}

void FrameSerializerV0::deserializeHeaderFrom(
    folly::io::Cursor& cur,
    FrameHeader& header,
    FrameFlags_V0& flags) {
  header.type_ = deserializeFrameType(cur.readBE<uint16_t>());

  flags = static_cast<FrameFlags_V0>(cur.readBE<uint16_t>());
  header.flags_ = deserializeFrameFlags(flags);

  header.streamId_ = cur.readBE<uint32_t>();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOutInternal(
    Frame_REQUEST_Base&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::FOLLOWS)) {
    extraFlags |= FrameFlags_V0::FOLLOWS;
  }
  if (!!(frame.header_.flags_ & FrameFlags::COMPLETE)) {
    extraFlags |= FrameFlags_V0::COMPLETE;
  }

  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, extraFlags);

  appender.writeBE<uint32_t>(frame.requestN_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_STREAM&& frame) {
  return serializeOutInternal(std::move(frame));
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_CHANNEL&& frame) {
  return serializeOutInternal(std::move(frame));
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_RESPONSE&& frame) {
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::FOLLOWS)) {
    extraFlags |= FrameFlags_V0::FOLLOWS;
  }

  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, extraFlags);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_FNF&& frame) {
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::FOLLOWS)) {
    extraFlags |= FrameFlags_V0::FOLLOWS;
  }

  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, extraFlags);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_N&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  appender.writeBE<uint32_t>(frame.requestN_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_METADATA_PUSH&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_CANCEL&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + (frame.metadata_ ? sizeof(uint32_t) : 0));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_PAYLOAD&& frame) {
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::FOLLOWS)) {
    extraFlags |= FrameFlags_V0::FOLLOWS;
  }
  if (!!(frame.header_.flags_ & FrameFlags::COMPLETE)) {
    extraFlags |= FrameFlags_V0::COMPLETE;
  }

  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, extraFlags);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_ERROR&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  appender.writeBE(static_cast<uint32_t>(frame.errorCode_));
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_KEEPALIVE&& frame,
    bool resumeable) {
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::KEEPALIVE_RESPOND)) {
    extraFlags |= FrameFlags_V0::KEEPALIVE_RESPOND;
  }

  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, extraFlags);
  // TODO: Remove hack:
  // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
  if (resumeable) {
    appender.writeBE(frame.position_);
  }
  if (frame.data_) {
    appender.insert(std::move(frame.data_));
  }
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_SETUP&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + 3 * sizeof(uint32_t) + frame.token_.data().size() +
      2 + frame.metadataMimeType_.length() + frame.dataMimeType_.length() +
      frame.payload_.framingSize());
  uint16_t extraFlags = 0;
  if (!!(frame.header_.flags_ & FrameFlags::RESUME_ENABLE)) {
    extraFlags |= FrameFlags_V0::RESUME_ENABLE;
  }
  if (!!(frame.header_.flags_ & FrameFlags::LEASE)) {
    extraFlags |= FrameFlags_V0::LEASE;
  }
  if (!!(frame.header_.flags_ & FrameFlags::STRICT)) {
    extraFlags |= FrameFlags_V0::STRICT;
  }

  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);

  serializeHeaderInto(appender, frame.header_, extraFlags);
  appender.writeBE(static_cast<uint16_t>(frame.versionMajor_));
  appender.writeBE(static_cast<uint16_t>(frame.versionMinor_));
  appender.writeBE(static_cast<uint32_t>(frame.keepaliveTime_));
  appender.writeBE(static_cast<uint32_t>(frame.maxLifetime_));

  // TODO: Remove hack:
  // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
  if (!!(frame.header_.flags_ & FrameFlags::RESUME_ENABLE)) {
    appender.push(frame.token_.data().data(), frame.token_.data().size());
  }

  CHECK(
      frame.metadataMimeType_.length() <= std::numeric_limits<uint8_t>::max());
  appender.writeBE(static_cast<uint8_t>(frame.metadataMimeType_.length()));
  appender.push(
      reinterpret_cast<const uint8_t*>(frame.metadataMimeType_.data()),
      frame.metadataMimeType_.length());

  CHECK(frame.dataMimeType_.length() <= std::numeric_limits<uint8_t>::max());
  appender.writeBE(static_cast<uint8_t>(frame.dataMimeType_.length()));
  appender.push(
      reinterpret_cast<const uint8_t*>(frame.dataMimeType_.data()),
      frame.dataMimeType_.length());

  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_LEASE&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + 3 * 2 * sizeof(uint32_t) +
      (frame.metadata_ ? sizeof(uint32_t) : 0));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  appender.writeBE(static_cast<uint32_t>(frame.ttl_));
  appender.writeBE(static_cast<uint32_t>(frame.numberOfRequests_));
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESUME&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(ResumeIdentificationToken) +
      sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  appender.push(frame.token_.data().data(), frame.token_.data().size());
  appender.writeBE(frame.position_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESUME_OK&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_, /*extraFlags=*/0);
  appender.writeBE(frame.position_);
  return queue.move();
}

bool FrameSerializerV0::deserializeFromInternal(
    Frame_REQUEST_Base& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::FOLLOWS)) {
      frame.header_.flags_ |= FrameFlags::FOLLOWS;
    }
    if (!!(flags & FrameFlags_V0::COMPLETE)) {
      frame.header_.flags_ |= FrameFlags::COMPLETE;
    }

    frame.requestN_ = cur.readBE<uint32_t>();
    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_STREAM& frame,
    std::unique_ptr<folly::IOBuf> in) {
  return deserializeFromInternal(frame, std::move(in));
}

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_CHANNEL& frame,
    std::unique_ptr<folly::IOBuf> in) {
  return deserializeFromInternal(frame, std::move(in));
}

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_RESPONSE& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::FOLLOWS)) {
      frame.header_.flags_ |= FrameFlags::FOLLOWS;
    }

    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_FNF& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::FOLLOWS)) {
      frame.header_.flags_ |= FrameFlags::FOLLOWS;
    }

    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_N& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.requestN_ = cur.readBE<uint32_t>();
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_METADATA_PUSH& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.metadata_ =
        Payload::deserializeMetadataFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return frame.metadata_ != nullptr;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_CANCEL& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.metadata_ =
        Payload::deserializeMetadataFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_PAYLOAD& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::FOLLOWS)) {
      frame.header_.flags_ |= FrameFlags::FOLLOWS;
    }
    if (!!(flags & FrameFlags_V0::COMPLETE)) {
      frame.header_.flags_ |= FrameFlags::COMPLETE;
    }

    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_ERROR& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.errorCode_ = static_cast<ErrorCode>(cur.readBE<uint32_t>());
    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_KEEPALIVE& frame,
    std::unique_ptr<folly::IOBuf> in,
    bool resumable) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::KEEPALIVE_RESPOND)) {
      frame.header_.flags_ |= FrameFlags::KEEPALIVE_RESPOND;
    }

    // TODO: Remove hack:
    // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
    if (resumable) {
      frame.position_ = cur.readBE<ResumePosition>();
    } else {
      frame.position_ = 0;
    }
    frame.data_ = Payload::deserializeDataFrom(cur);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_SETUP& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);

    if (!!(flags & FrameFlags_V0::RESUME_ENABLE)) {
      frame.header_.flags_ |= FrameFlags::RESUME_ENABLE;
    }
    if (!!(flags & FrameFlags_V0::LEASE)) {
      frame.header_.flags_ |= FrameFlags::LEASE;
    }
    if (!!(flags & FrameFlags_V0::STRICT)) {
      frame.header_.flags_ |= FrameFlags::STRICT;
    }

    frame.versionMajor_ = cur.readBE<uint16_t>();
    frame.versionMinor_ = cur.readBE<uint16_t>();
    frame.keepaliveTime_ = cur.readBE<uint32_t>();
    frame.maxLifetime_ = cur.readBE<uint32_t>();

    // TODO: Remove hack:
    // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
    if (!!(frame.header_.flags_ & FrameFlags::RESUME_ENABLE)) {
      ResumeIdentificationToken::Data data;
      cur.pull(data.data(), data.size());
      frame.token_.set(std::move(data));
    } else {
      frame.token_ = ResumeIdentificationToken();
    }

    auto mdmtLen = cur.readBE<uint8_t>();
    frame.metadataMimeType_ = cur.readFixedString(mdmtLen);

    auto dmtLen = cur.readBE<uint8_t>();
    frame.dataMimeType_ = cur.readFixedString(dmtLen);
    frame.payload_.deserializeFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_LEASE& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.ttl_ = cur.readBE<uint32_t>();
    frame.numberOfRequests_ = cur.readBE<uint32_t>();
    frame.metadata_ =
        Payload::deserializeMetadataFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_RESUME& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    ResumeIdentificationToken::Data data;
    cur.pull(data.data(), data.size());
    frame.token_.set(std::move(data));
    frame.position_ = cur.readBE<ResumePosition>();
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_RESUME_OK& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    FrameFlags_V0 flags;
    deserializeHeaderFrom(cur, frame.header_, flags);
    frame.position_ = cur.readBE<ResumePosition>();
  } catch (...) {
    return false;
  }
  return true;
}

} // reactivesocket
