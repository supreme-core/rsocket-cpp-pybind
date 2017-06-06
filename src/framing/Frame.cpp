// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/framing/Frame.h"
#include <folly/Memory.h>
#include <folly/Optional.h>
#include <folly/io/Cursor.h>
#include <bitset>
#include "src/RSocketParameters.h"

namespace rsocket {

const uint32_t Frame_LEASE::kMaxTtl;
const uint32_t Frame_LEASE::kMaxNumRequests;
const uint32_t Frame_SETUP::kMaxKeepaliveTime;
const uint32_t Frame_SETUP::kMaxLifetime;

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

std::string to_string(FrameType type) {
  switch (type) {
    case FrameType::REQUEST_STREAM:
      return "REQUEST_STREAM";
    case FrameType::REQUEST_CHANNEL:
      return "REQUEST_CHANNEL";
    case FrameType::REQUEST_N:
      return "REQUEST_N";
    case FrameType::REQUEST_RESPONSE:
      return "REQUEST_RESPONSE";
    case FrameType::REQUEST_FNF:
      return "REQUEST_FNF";
    case FrameType::CANCEL:
      return "CANCEL";
    case FrameType::PAYLOAD:
      return "PAYLOAD";
    case FrameType::ERROR:
      return "ERROR";
    case FrameType::RESERVED:
      return "RESERVED";
    case FrameType::KEEPALIVE:
      return "KEEPALIVE";
    case FrameType::SETUP:
      return "SETUP";
    case FrameType::LEASE:
      return "LEASE";
    case FrameType::METADATA_PUSH:
      return "METADATA_PUSH";
    case FrameType::RESUME:
      return "RESUME";
    case FrameType::RESUME_OK:
      return "RESUME_OK";
    case FrameType::EXT:
      return "EXT";
  }
  // this should be never hit because the switch is over all cases
  LOG(FATAL) << "unknown FrameType=" << static_cast<int>(type);
}

std::ostream& operator<<(std::ostream& os, FrameType type) {
  return os << to_string(type);
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

std::ostream& operator<<(
    std::ostream& os,
    const std::pair<FrameFlags, FrameFlags>& frameFlagsAndAllowedFlags) {
  FrameFlags frameFlags = frameFlagsAndAllowedFlags.first;
  FrameFlags allowedFlags = frameFlagsAndAllowedFlags.second;
  FrameFlags foundFlags = FrameFlags::EMPTY;

  std::stringstream ss;
  std::string delimeter = "";
  if (!!(allowedFlags & FrameFlags::IGNORE)
      && !!(frameFlags & FrameFlags::IGNORE)) {
    ss << delimeter << "IGNORE";
    delimeter = "|";
    foundFlags |= FrameFlags::IGNORE;
  }
  if (!!(allowedFlags & FrameFlags::METADATA)
      && !!(frameFlags & FrameFlags::METADATA)) {
    ss << delimeter << "METADATA";
    delimeter = "|";
    foundFlags |= FrameFlags::METADATA;
  }
  if (!!(allowedFlags & FrameFlags::RESUME_ENABLE)
      && !!(frameFlags & FrameFlags::RESUME_ENABLE)) {
    ss << delimeter << "RESUME_ENABLE";
    delimeter = "|";
    foundFlags |= FrameFlags::RESUME_ENABLE;
  }
  if (!!(allowedFlags & FrameFlags::LEASE)
      && !!(frameFlags & FrameFlags::LEASE)) {
    ss << delimeter << "LEASE";
    delimeter = "|";
    foundFlags |= FrameFlags::LEASE;
  }
  if (!!(allowedFlags & FrameFlags::KEEPALIVE_RESPOND)
      && !!(frameFlags & FrameFlags::KEEPALIVE_RESPOND)) {
    ss << delimeter << "KEEPALIVE_RESPOND";
    delimeter = "|";
    foundFlags |= FrameFlags::KEEPALIVE_RESPOND;
  }
  if (!!(allowedFlags & FrameFlags::FOLLOWS)
      && !!(frameFlags & FrameFlags::FOLLOWS)) { // FOLLOWS = 0x80,
    ss << delimeter << "FOLLOWS";
    delimeter = "|";
    foundFlags |= FrameFlags::FOLLOWS;
  }
  if (!!(allowedFlags & FrameFlags::COMPLETE)
      && !!(frameFlags & FrameFlags::COMPLETE)) { // COMPLETE = 0x40
    ss << delimeter << "COMPLETE";
    delimeter = "|";
    foundFlags |= FrameFlags::COMPLETE;
  }
  if (!!(allowedFlags & FrameFlags::NEXT)
      && !!(frameFlags & FrameFlags::NEXT)) { // NEXT = 0x20
    ss << "NEXT";
    delimeter = "|";
    foundFlags |= FrameFlags::NEXT;
  }

  if (foundFlags != frameFlags) {
    return operator<<(os, frameFlags);
  }
  if (!delimeter.empty()) {
    return os << ss.str();
  }
  return os << "EMPTY";
}

std::ostream& operator<<(std::ostream& os, FrameFlags frameFlags) {
  // TODO Match the Flag names with the AllowedFlags in the Frame declarations
  std::bitset<16> flags(static_cast<uint16_t>(frameFlags));
  return os << flags;
}

std::ostream& operator<<(std::ostream& os, const std::pair<const FrameHeader&, FrameFlags>& headerAndAllowedFlags) {
  const auto& header = headerAndAllowedFlags.first;
  return os << header.type_ << "["
            << std::make_pair(header.flags_, headerAndAllowedFlags.second)
            << ", " << header.streamId_ << "]";
}

std::ostream& operator<<(std::ostream& os, const FrameHeader& header) {
  return os << header.type_ << "[" << header.flags_ << ", " << header.streamId_
            << "]";
}
/// @}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_Base& frame) {
  return os << std::make_pair(frame.header_, FrameFlags::EMPTY)
            << "(" << frame.requestN_ << ", "
            << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_N& frame) {
  return os << std::make_pair(frame.header_, FrameFlags::EMPTY)
            << "(" << frame.requestN_ << ")";
}

constexpr FrameFlags Frame_REQUEST_RESPONSE::AllowedFlags;
std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_RESPONSE& frame) {
  return os << std::make_pair(frame.header_, Frame_REQUEST_RESPONSE::AllowedFlags)
            << ", " << frame.payload_;
}

constexpr FrameFlags Frame_REQUEST_FNF::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_FNF& frame) {
  return os << std::make_pair(frame.header_, Frame_REQUEST_FNF::AllowedFlags)
            << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_METADATA_PUSH& frame) {
  return os << std::make_pair(frame.header_, FrameFlags::METADATA) << ", "
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength()
                                : 0);
}

std::ostream& operator<<(std::ostream& os, const Frame_CANCEL& frame) {
  return os << std::make_pair(frame.header_, FrameFlags::EMPTY);
}

Frame_PAYLOAD Frame_PAYLOAD::complete(StreamId streamId) {
  return Frame_PAYLOAD(streamId, FrameFlags::COMPLETE, Payload());
}

constexpr FrameFlags Frame_PAYLOAD::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_PAYLOAD& frame) {
  return os << std::make_pair(frame.header_, Frame_PAYLOAD::AllowedFlags)
            << ", (" << frame.payload_;
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

Frame_ERROR Frame_ERROR::error(StreamId streamId, Payload&& payload) {
  DCHECK(streamId) << "streamId MUST be non-0";
  return Frame_ERROR(streamId, ErrorCode::INVALID, std::move(payload));
}

Frame_ERROR Frame_ERROR::applicationError(
    StreamId streamId,
    Payload&& payload) {
  DCHECK(streamId) << "streamId MUST be non-0";
  return Frame_ERROR(
      streamId, ErrorCode::APPLICATION_ERROR, std::move(payload));
}

constexpr FrameFlags Frame_ERROR::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_ERROR& frame) {
  return os << std::make_pair(frame.header_, Frame_ERROR::AllowedFlags)
            << ", " << frame.errorCode_ << ", "
            << frame.payload_;
}

constexpr FrameFlags Frame_KEEPALIVE::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_KEEPALIVE& frame) {
  return os << std::make_pair(frame.header_, Frame_KEEPALIVE::AllowedFlags)
            << "(<"
            << (frame.data_ ? frame.data_->computeChainDataLength() : 0)
            << ">)";
}

constexpr FrameFlags Frame_SETUP::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_SETUP& frame) {
  return os << std::make_pair(frame.header_, Frame_SETUP::AllowedFlags)
            << ", Version: " << frame.versionMajor_ << "." << frame.versionMinor_
            << ", (" << frame.payload_;
}

void Frame_SETUP::moveToSetupPayload(SetupParameters& setupPayload) {
  setupPayload.metadataMimeType = std::move(metadataMimeType_);
  setupPayload.dataMimeType = std::move(dataMimeType_);
  setupPayload.payload = std::move(payload_);
  setupPayload.token = std::move(token_);
  setupPayload.resumable = !!(header_.flags_ & FrameFlags::RESUME_ENABLE);
  setupPayload.protocolVersion = ProtocolVersion(versionMajor_, versionMinor_);
}

constexpr FrameFlags Frame_LEASE::AllowedFlags;
std::ostream& operator<<(std::ostream& os, const Frame_LEASE& frame) {
  return os << std::make_pair(frame.header_, Frame_LEASE::AllowedFlags)
            << ", ("
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength() : 0)
            << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME& frame) {
  return os << std::make_pair(frame.header_, FrameFlags::EMPTY)
            << ", ("
            << "token"
            << ", @server " << frame.lastReceivedServerPosition_ << ", @client "
            << frame.clientPosition_ << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME_OK& frame) {
  return os << frame.header_ << ", (@" << frame.position_ << ")";
}

constexpr FrameFlags Frame_REQUEST_CHANNEL::AllowedFlags;
std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_CHANNEL& frame) {
  return os << std::make_pair(frame.header_, Frame_REQUEST_CHANNEL::AllowedFlags)
            << ", " << frame.payload_;
}

} // reactivesocket
