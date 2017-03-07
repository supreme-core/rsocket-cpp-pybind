// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v0.h"

#include <folly/io/Cursor.h>

namespace reactivesocket {

constexpr const ProtocolVersion FrameSerializerV0::Version;

static folly::IOBufQueue createBufferQueue(size_t bufferSize) {
  auto buf = reactivesocket::FrameBufferAllocator::allocate(bufferSize);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  queue.append(std::move(buf));
  return queue;
}

ProtocolVersion FrameSerializerV0::protocolVersion() {
  return Version;
}

static FrameType deserializeFrameType(uint16_t frameType) {
  if (frameType > static_cast<uint16_t>(FrameType::RESUME_OK)) {
    return FrameType::RESERVED;
  }

  constexpr static const auto REQUEST_SUB = 0x0007;
  if (frameType == REQUEST_SUB) {
    return FrameType::REQUEST_STREAM;
  }

  return static_cast<FrameType>(frameType);
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
    const FrameHeader& header) {
  appender.writeBE<uint16_t>(static_cast<uint16_t>(header.type_));
  appender.writeBE<uint16_t>(header.flags_);
  appender.writeBE<uint32_t>(header.streamId_);
}

void FrameSerializerV0::deserializeHeaderFrom(
    folly::io::Cursor& cur,
    FrameHeader& header) {
  header.type_ = deserializeFrameType(cur.readBE<uint16_t>());
  header.flags_ = cur.readBE<uint16_t>();
  header.streamId_ = cur.readBE<uint32_t>();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOutInternal(
    Frame_REQUEST_Base&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
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
  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_FNF&& frame) {
  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_N&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  appender.writeBE<uint32_t>(frame.requestN_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_METADATA_PUSH&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_CANCEL&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + (frame.metadata_ ? sizeof(uint32_t) : 0));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESPONSE&& frame) {
  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_ERROR&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  appender.writeBE(static_cast<uint32_t>(frame.errorCode_));
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_KEEPALIVE&& frame,
    bool resumeable) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
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
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);

  serializeHeaderInto(appender, frame.header_);
  appender.writeBE(static_cast<uint16_t>(frame.versionMajor_));
  appender.writeBE(static_cast<uint16_t>(frame.versionMinor_));
  appender.writeBE(static_cast<uint32_t>(frame.keepaliveTime_));
  appender.writeBE(static_cast<uint32_t>(frame.maxLifetime_));

  // TODO: Remove hack:
  // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
  if (frame.header_.flags_ & FrameFlags_RESUME_ENABLE) {
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
  serializeHeaderInto(appender, frame.header_);
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
  serializeHeaderInto(appender, frame.header_);
  appender.push(frame.token_.data().data(), frame.token_.data().size());
  appender.writeBE(frame.position_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESUME_OK&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  serializeHeaderInto(appender, frame.header_);
  appender.writeBE(frame.position_);
  return queue.move();
}

bool FrameSerializerV0::deserializeFromInternal(
    Frame_REQUEST_Base& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
    frame.metadata_ =
        Payload::deserializeMetadataFrom(cur, frame.header_.flags_);
  } catch (...) {
    return false;
  }
  return true;
}

bool FrameSerializerV0::deserializeFrom(
    Frame_RESPONSE& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
    if (frame.header_.flags_ & FrameFlags_METADATA) {
      return false;
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
    deserializeHeaderFrom(cur, frame.header_);
    frame.versionMajor_ = cur.readBE<uint16_t>();
    frame.versionMinor_ = cur.readBE<uint16_t>();
    frame.keepaliveTime_ = cur.readBE<uint32_t>();
    frame.maxLifetime_ = cur.readBE<uint32_t>();

    // TODO: Remove hack:
    // https://github.com/ReactiveSocket/reactivesocket-cpp/issues/243
    if (frame.header_.flags_ & FrameFlags_RESUME_ENABLE) {
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
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
    deserializeHeaderFrom(cur, frame.header_);
    frame.position_ = cur.readBE<ResumePosition>();
  } catch (...) {
    return false;
  }
  return true;
}

} // reactivesocket
