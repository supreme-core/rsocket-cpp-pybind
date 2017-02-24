// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/Frame.h"
#include <folly/Memory.h>
#include <folly/Optional.h>
#include <folly/io/Cursor.h>
#include <bitset>
#include "src/ConnectionSetupPayload.h"

// TODO(stupaq): strict enum validation
// TODO(stupaq): verify whether frames contain extra data
// TODO(stupaq): get rid of these try-catch blocks
namespace reactivesocket {

std::unique_ptr<folly::IOBuf> FrameBufferAllocator::allocate(size_t size) {
  // Purposely leak the allocator, since it's hard to deterministically
  // guarantee that threads will stop using it before it would get statically
  // destructed.
  static auto* singleton = new FrameBufferAllocator;
  return singleton->allocateBuffer(size);
}

std::unique_ptr<folly::IOBuf> FrameBufferAllocator::allocateBuffer(
    size_t size) {
  return folly::IOBuf::createCombined(size);
}

std::ostream& operator<<(std::ostream& os, FrameType type) {
  switch (type) {
    case FrameType::REQUEST_STREAM:
      return os << "REQUEST_STREAM";
    case FrameType::REQUEST_SUB:
      return os << "REQUEST_SUB";
    case FrameType::REQUEST_CHANNEL:
      return os << "REQUEST_CHANNEL";
    case FrameType::REQUEST_N:
      return os << "REQUEST_N";
    case FrameType::REQUEST_RESPONSE:
      return os << "REQUEST_RESPONSE";
    case FrameType::REQUEST_FNF:
      return os << "REQUEST_FNF";
    case FrameType::CANCEL:
      return os << "CANCEL";
    case FrameType::RESPONSE:
      return os << "RESPONSE";
    case FrameType::ERROR:
      return os << "ERROR";
    case FrameType::RESERVED:
      return os << "RESERVED";
    case FrameType::KEEPALIVE:
      return os << "KEEPALIVE";
    case FrameType::SETUP:
      return os << "SETUP";
    case FrameType::LEASE:
      return os << "LEASE";
    case FrameType::METADATA_PUSH:
      return os << "METADATA_PUSH";
    case FrameType::RESUME:
      return os << "RESUME";
    case FrameType::RESUME_OK:
      return os << "RESUME_OK";
  }
  return os << "FrameType(" << static_cast<uint16_t>(type) << ")";
}

std::ostream& operator<<(std::ostream& os, ErrorCode errorCode) {
  switch (errorCode) {
    case ErrorCode::RESERVED:
      return os << "RESERVED";
    case ErrorCode::APPLICATION_ERROR:
      return os << "APPLICATION_ERROR";
    case ErrorCode::REJECTED:
      return os << "REJECTED";
    case ErrorCode::CANCELED:
      return os << "CANCELED";
    case ErrorCode::INVALID:
      return os << "INVALID";
    case ErrorCode::INVALID_SETUP:
      return os << "INVALID_SETUP";
    case ErrorCode::REJECTED_SETUP:
      return os << "REJECTED_SETUP";
    case ErrorCode::UNSUPPORTED_SETUP:
      return os << "UNSUPPORTED_SETUP";
    case ErrorCode::CONNECTION_ERROR:
      return os << "CONNECTION_ERROR";
  }
  return os << "ErrorCode(" << static_cast<uint32_t>(errorCode) << ")";
}

std::ostream& operator<<(std::ostream& os, const FrameHeader& header) {
  std::bitset<16> flags(header.flags_);
  return os << header.type_ << "[" << flags << ", " << header.streamId_ << "]";
}
/// @}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_Base& frame) {
  return os << frame.header_ << "(" << frame.requestN_ << ", "
            << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_N& frame) {
  return os << frame.header_ << "(" << frame.requestN_ << ")";
}

std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_RESPONSE& frame) {
  return os << frame.header_ << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_FNF& frame) {
  return os << frame.header_ << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_METADATA_PUSH& frame) {
  return os << frame.header_ << ", "
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength()
                                : 0);
}

std::ostream& operator<<(std::ostream& os, const Frame_CANCEL& frame) {
  return os << frame.header_ << ", "
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength()
                                : 0);
}

Frame_RESPONSE Frame_RESPONSE::complete(StreamId streamId) {
  return Frame_RESPONSE(streamId, FrameFlags_COMPLETE, Payload());
}

std::ostream& operator<<(std::ostream& os, const Frame_RESPONSE& frame) {
  return os << frame.header_ << ", (" << frame.payload_;
}

Frame_ERROR Frame_ERROR::unexpectedFrame() {
  return Frame_ERROR(
      0, ErrorCode::CONNECTION_ERROR, Payload("unexpected frame"));
}

Frame_ERROR Frame_ERROR::invalidFrame() {
  return Frame_ERROR(0, ErrorCode::CONNECTION_ERROR, Payload("invalid frame"));
}

Frame_ERROR Frame_ERROR::badSetupFrame(const std::string& message) {
  return Frame_ERROR(0, ErrorCode::INVALID_SETUP, Payload(message));
}

Frame_ERROR Frame_ERROR::connectionError(const std::string& message) {
  return Frame_ERROR(0, ErrorCode::CONNECTION_ERROR, Payload(message));
}

Frame_ERROR Frame_ERROR::invalid(
    StreamId streamId,
    const std::string& message) {
  DCHECK(streamId) << "streamId MUST be non-0";
  return Frame_ERROR(streamId, ErrorCode::INVALID, Payload(message));
}

Frame_ERROR Frame_ERROR::applicationError(
    StreamId streamId,
    const std::string& message) {
  DCHECK(streamId) << "streamId MUST be non-0";
  return Frame_ERROR(streamId, ErrorCode::APPLICATION_ERROR, Payload(message));
}

std::ostream& operator<<(std::ostream& os, const Frame_ERROR& frame) {
  return os << frame.header_ << ", " << frame.errorCode_ << ", "
            << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_KEEPALIVE& frame) {
  return os << frame.header_ << "(<"
            << (frame.data_ ? frame.data_->computeChainDataLength() : 0)
            << ">)";
}

std::ostream& operator<<(std::ostream& os, const Frame_SETUP& frame) {
  return os << frame.header_ << ", (" << frame.payload_;
}

void Frame_SETUP::moveToSetupPayload(ConnectionSetupPayload& setupPayload) {
  setupPayload.metadataMimeType = std::move(metadataMimeType_);
  setupPayload.dataMimeType = std::move(dataMimeType_);
  setupPayload.payload = std::move(payload_);
  setupPayload.token = std::move(token_);
  setupPayload.resumable = header_.flags_ & FrameFlags_RESUME_ENABLE;
}

std::ostream& operator<<(std::ostream& os, const Frame_LEASE& frame) {
  return os << frame.header_ << ", ("
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength() : 0)
            << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME& frame) {
  return os << frame.header_ << ", ("
            << "token"
            << ", @" << frame.position_ << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME_OK& frame) {
  return os << frame.header_ << ", (@" << frame.position_ << ")";
}
} // reactivesocket
