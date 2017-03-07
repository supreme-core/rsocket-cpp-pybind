// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Optional.h>
#include <iosfwd>
#include <memory>
#include <string>
#include "src/Frame.h"

// bug in GCC: https://bugzilla.redhat.com/show_bug.cgi?id=130601
#pragma push_macro("major")
#pragma push_macro("minor")
#undef major
#undef minor

namespace reactivesocket {

struct ProtocolVersion {
  uint16_t major{};
  uint16_t minor{};

  constexpr ProtocolVersion() = default;
  constexpr ProtocolVersion(uint16_t _major, uint16_t _minor)
      : major(_major), minor(_minor) {}

  bool operator>(const ProtocolVersion& other) const {
    return major > other.major || (major == other.major && minor > other.minor);
  }

  bool operator==(const ProtocolVersion& other) const {
    return major == other.major && minor == other.minor;
  }

  bool operator!=(const ProtocolVersion& other) const {
    return !((*this) == other);
  }
};
std::ostream& operator<<(std::ostream&, const ProtocolVersion&);

#pragma pop_macro("major")
#pragma pop_macro("minor")

// interface separating serialization/deserialization of ReactiveSocket frames
class FrameSerializer {
 public:
  virtual ~FrameSerializer() = default;

  virtual ProtocolVersion protocolVersion() = 0;

  static std::unique_ptr<FrameSerializer> createFrameSerializer(
      const ProtocolVersion& protocolVersion);
  static std::unique_ptr<FrameSerializer> createCurrentVersion();

  virtual FrameType peekFrameType(const folly::IOBuf& in) = 0;
  virtual folly::Optional<StreamId> peekStreamId(const folly::IOBuf& in) = 0;

  constexpr static const ProtocolVersion kCurrentProtocolVersion =
      ProtocolVersion(0, 1);

  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_STREAM&&) = 0;
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
