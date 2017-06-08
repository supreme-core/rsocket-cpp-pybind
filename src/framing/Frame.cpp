// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/framing/Frame.h"
#include <bitset>
#include <sstream>
#include <folly/Memory.h>
#include <folly/Optional.h>
#include <folly/io/Cursor.h>
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

constexpr const char sFrameFlags_EMPTY[] = "EMPTY";
constexpr const char sFrameFlags_METADATA[] = "METADATA";
constexpr const char sFrameFlags_RESUME_ENABLE[] = "RESUME_ENABLE";
constexpr const char sFrameFlags_LEASE[] = "LEASE";
constexpr const char sFrameFlags_KEEPALIVE_RESPOND[] = "KEEPALIVE_RESPOND";
constexpr const char sFrameFlags_FOLLOWS[] = "FOLLOWS";
constexpr const char sFrameFlags_COMPLETE[] = "COMPLETE";
constexpr const char sFrameFlags_NEXT[] = "NEXT";

std::string to_string(FrameFlags frameFlags,
                      const std::vector<std::pair<FrameFlags, std::string>>& allowedFlags)
{
  FrameFlags foundFlags = FrameFlags::EMPTY;

  std::stringstream ss;
  std::string delimeter = "";
  for (auto& pair : allowedFlags) {
    if (!!(frameFlags & pair.first)) {
      ss << delimeter << pair.second;
      delimeter = "|";
      foundFlags |= pair.first;
    }
  }

  if (foundFlags != frameFlags) {
    ss <<frameFlags;
  } else if (delimeter.empty()) {
    ss << sFrameFlags_EMPTY;
  }
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, FrameFlags frameFlags) {
  std::bitset<16> flags(static_cast<uint16_t>(frameFlags));
  return os << flags;
}

std::ostream& operator<<(std::ostream& os, const FrameHeader& header) {
  return os << header.type_ << "[" << header.flags_ << ", " << header.streamId_
            << "]";
}

std::ostream& operator<<(std::ostream& os,
    const std::pair< const FrameHeader&,
        const std::vector<std::pair<FrameFlags, std::string>>&
      > & headerAndAllowedFlags
  ) {
  const auto& header = headerAndAllowedFlags.first;
  const auto& allowedFlags = headerAndAllowedFlags.second;
  return os << header.type_ << "[" << to_string(header.flags_, allowedFlags)
            << ", " << header.streamId_ << "]";
}

/// @}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_Base& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>> {})
            << "("
            << frame.requestN_ << ", "
            << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_N& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>> {})
            << "(" << frame.requestN_ << ")";
}

std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_RESPONSE& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::FOLLOWS, sFrameFlags_FOLLOWS} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_REQUEST_FNF& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::FOLLOWS, sFrameFlags_FOLLOWS} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_METADATA_PUSH& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>>{})
            << ", "
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength()
                                : 0);
}

std::ostream& operator<<(std::ostream& os, const Frame_CANCEL& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>> {});
}

Frame_PAYLOAD Frame_PAYLOAD::complete(StreamId streamId) {
  return Frame_PAYLOAD(streamId, FrameFlags::COMPLETE, Payload());
}

std::ostream& operator<<(std::ostream& os, const Frame_PAYLOAD& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::FOLLOWS, sFrameFlags_FOLLOWS},
      {FrameFlags::COMPLETE, sFrameFlags_COMPLETE},
      {FrameFlags::NEXT, sFrameFlags_NEXT} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", "
            << frame.header_.streamId_
            << "], (" << frame.payload_;
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

std::ostream& operator<<(std::ostream& os, const Frame_ERROR& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", " << frame.errorCode_
            << ", " << frame.payload_;
}

std::ostream& operator<<(std::ostream& os, const Frame_KEEPALIVE& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::KEEPALIVE_RESPOND, sFrameFlags_KEEPALIVE_RESPOND} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << "(<"
            << (frame.data_ ? frame.data_->computeChainDataLength() : 0)
            << ">)";
}

std::ostream& operator<<(std::ostream& os, const Frame_SETUP& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::RESUME_ENABLE, sFrameFlags_RESUME_ENABLE},
      {FrameFlags::LEASE, sFrameFlags_LEASE} };
  return os << std::make_pair(frame.header_, allowedFlags)
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

std::ostream& operator<<(std::ostream& os, const Frame_LEASE& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", ("
            << (frame.metadata_ ? frame.metadata_->computeChainDataLength() : 0)
            << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>>{})
            << ", ("
            << "token"
            << ", @server " << frame.lastReceivedServerPosition_ << ", @client "
            << frame.clientPosition_ << ")";
}

std::ostream& operator<<(std::ostream& os, const Frame_RESUME_OK& frame) {
  return os << std::make_pair(frame.header_,
                              std::vector<std::pair<FrameFlags, std::string>>{})
            << ", (@" << frame.position_ << ")";
}

std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_CHANNEL& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::FOLLOWS, sFrameFlags_FOLLOWS},
      {FrameFlags::COMPLETE, sFrameFlags_COMPLETE} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", " << frame.payload_;
}

std::ostream& operator<<(
    std::ostream& os,
    const Frame_REQUEST_STREAM& frame) {
  static const std::vector<std::pair<FrameFlags, std::string>> allowedFlags {
      {FrameFlags::METADATA, sFrameFlags_METADATA},
      {FrameFlags::FOLLOWS, sFrameFlags_FOLLOWS} };
  return os << std::make_pair(frame.header_, allowedFlags)
            << ", " << frame.payload_;
}

} // reactivesocket
