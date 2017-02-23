// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/versions/FrameSerializer_v0.h"

#include <folly/io/Cursor.h>

namespace {
static folly::IOBufQueue createBufferQueue(size_t bufferSize) {
  auto buf = reactivesocket::FrameBufferAllocator::allocate(bufferSize);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  queue.append(std::move(buf));
  return queue;
}
}

namespace reactivesocket {

std::string FrameSerializerV0::protocolVersion() {
  return "0.0";
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOutInternal(
    Frame_REQUEST_Base&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE<uint32_t>(frame.requestN_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_STREAM&& frame) {
  return serializeOutInternal(std::move(frame));
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_SUB&& frame) {
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
  frame.header_.serializeInto(appender);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_FNF&& frame) {
  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_N&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE<uint32_t>(frame.requestN_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_METADATA_PUSH&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(uint32_t));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_CANCEL&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + (frame.metadata_ ? sizeof(uint32_t) : 0));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  Payload::serializeMetadataInto(appender, std::move(frame.metadata_));
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESPONSE&& frame) {
  auto queue =
      createBufferQueue(FrameHeader::kSize + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_ERROR&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE(static_cast<uint32_t>(frame.errorCode_));
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_KEEPALIVE&& frame,
    bool resumeable) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
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

  frame.header_.serializeInto(appender);
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
  frame.header_.serializeInto(appender);
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
  frame.header_.serializeInto(appender);
  appender.push(frame.token_.data().data(), frame.token_.data().size());
  appender.writeBE(frame.position_);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_RESUME_OK&& frame) {
  auto queue = createBufferQueue(FrameHeader::kSize + sizeof(ResumePosition));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE(frame.position_);
  return queue.move();
}

bool FrameSerializerV0::deserializeFromInternal(
    Frame_REQUEST_Base& frame,
    std::unique_ptr<folly::IOBuf> in) {
  folly::io::Cursor cur(in.get());
  try {
    frame.header_.deserializeFrom(cur);
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
    Frame_REQUEST_SUB& frame,
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
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
    frame.header_.deserializeFrom(cur);
    frame.position_ = cur.readBE<ResumePosition>();
  } catch (...) {
    return false;
  }
  return true;
}

} // reactivesocket
