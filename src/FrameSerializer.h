// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Optional.h>
#include <memory>
#include <string>
#include "src/Frame.h"

namespace reactivesocket {

// interface separating serialization/deserialization of ReactiveSocket frames
class FrameSerializer {
 public:
  virtual ~FrameSerializer() = default;

  virtual std::string protocolVersion() = 0;

  static std::unique_ptr<FrameSerializer> createFrameSerializer(
      std::string protocolVersion);
  static std::unique_ptr<FrameSerializer> createCurrentVersion();

  virtual FrameType peekFrameType(const folly::IOBuf& in) = 0;
  virtual folly::Optional<StreamId> peekStreamId(const folly::IOBuf& in) = 0;

  constexpr static const char* kCurrentProtocolVersion = "0.1";
  constexpr static const uint16_t kCurrentProtocolVersionMajor = 0;
  constexpr static const uint16_t kCurrentProtocolVersionMinor = 1;

  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_STREAM&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_SUB&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_CHANNEL&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_RESPONSE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_FNF&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_N&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_METADATA_PUSH&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESPONSE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_KEEPALIVE&&,
      bool) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME_OK&&) = 0;

  virtual bool deserializeFrom(
      Frame_REQUEST_STREAM&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_SUB&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_CHANNEL&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_RESPONSE&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_FNF&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_N&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_METADATA_PUSH&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_CANCEL&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESPONSE&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(Frame_ERROR&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_KEEPALIVE&,
      std::unique_ptr<folly::IOBuf>,
      bool supportsResumability) = 0;
  virtual bool deserializeFrom(Frame_SETUP&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(Frame_LEASE&, std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESUME&,
      std::unique_ptr<folly::IOBuf>) = 0;
  virtual bool deserializeFrom(
      Frame_RESUME_OK&,
      std::unique_ptr<folly::IOBuf>) = 0;
};

} // reactivesocket
