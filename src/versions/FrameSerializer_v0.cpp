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

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_STREAM&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE<uint32_t>(frame.requestN_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_SUB&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE<uint32_t>(frame.requestN_);
  frame.payload_.serializeInto(appender);
  return queue.move();
}

std::unique_ptr<folly::IOBuf> FrameSerializerV0::serializeOut(
    Frame_REQUEST_CHANNEL&& frame) {
  auto queue = createBufferQueue(
      FrameHeader::kSize + sizeof(uint32_t) + frame.payload_.framingSize());
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  frame.header_.serializeInto(appender);
  appender.writeBE<uint32_t>(frame.requestN_);
  frame.payload_.serializeInto(appender);
  return queue.move();
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

bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_STREAM&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_SUB&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_CHANNEL&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_RESPONSE&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_FNF&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_REQUEST_N&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_METADATA_PUSH&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_CANCEL&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
  ;
}
bool FrameSerializerV0::deserializeFrom(
    Frame_RESPONSE&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_ERROR&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_KEEPALIVE&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_SETUP& frame,
    std::unique_ptr<folly::IOBuf> in) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_LEASE&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_RESUME&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}
bool FrameSerializerV0::deserializeFrom(
    Frame_RESUME_OK&,
    std::unique_ptr<folly::IOBuf>) {
  throw std::runtime_error("v0 serialization not implemented");
}

} // reactivesocket
