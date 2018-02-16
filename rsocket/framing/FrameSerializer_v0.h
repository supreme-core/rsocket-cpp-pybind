// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/framing/FrameSerializer.h"

namespace rsocket {

class FrameSerializerV0 : public FrameSerializer {
 public:
  constexpr static const ProtocolVersion Version = ProtocolVersion(0, 0);
  constexpr static const size_t kFrameHeaderSize = 8; // bytes

  ProtocolVersion protocolVersion() const override;

  FrameType peekFrameType(const folly::IOBuf& in) const override;
  folly::Optional<StreamId> peekStreamId(const folly::IOBuf& in) const override;

  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_STREAM&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_CHANNEL&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_RESPONSE&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_FNF&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_N&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_METADATA_PUSH&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_PAYLOAD&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_KEEPALIVE&&, bool) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) const override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME_OK&&) const override;

  bool deserializeFrom(Frame_REQUEST_STREAM&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(Frame_REQUEST_CHANNEL&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(Frame_REQUEST_RESPONSE&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(Frame_REQUEST_FNF&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(Frame_REQUEST_N&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(Frame_METADATA_PUSH&, std::unique_ptr<folly::IOBuf>)
      const override;
  bool deserializeFrom(
      Frame_CANCEL&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(
      Frame_PAYLOAD&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(
      Frame_ERROR&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(Frame_KEEPALIVE&, std::unique_ptr<folly::IOBuf>, bool)
      const override;
  bool deserializeFrom(
      Frame_SETUP&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(
      Frame_LEASE&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(
      Frame_RESUME&, std::unique_ptr<folly::IOBuf>) const override;
  bool deserializeFrom(Frame_RESUME_OK&, std::unique_ptr<folly::IOBuf>)
      const override;

  static std::unique_ptr<folly::IOBuf> deserializeMetadataFrom(
      folly::io::Cursor& cur,
      FrameFlags flags);

 private:
  std::unique_ptr<folly::IOBuf> serializeOutInternal(
      Frame_REQUEST_Base&& frame) const;

  size_t frameLengthFieldSize() const override;
};
} // namespace rsocket
