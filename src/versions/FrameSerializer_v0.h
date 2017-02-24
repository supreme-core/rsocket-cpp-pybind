// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/FrameSerializer.h"

namespace reactivesocket {

class FrameSerializerV0 : public FrameSerializer {
 public:
  std::string protocolVersion() override;

  FrameType peekFrameType(const folly::IOBuf& in) override;
  folly::Optional<StreamId> peekStreamId(const folly::IOBuf& in) override;

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_STREAM&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_SUB&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_CHANNEL&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_RESPONSE&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_FNF&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_N&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_METADATA_PUSH&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESPONSE&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_KEEPALIVE&&, bool) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) override;
  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME_OK&&) override;

  bool deserializeFrom(Frame_REQUEST_STREAM&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_REQUEST_SUB&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_REQUEST_CHANNEL&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_REQUEST_RESPONSE&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_REQUEST_FNF&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_REQUEST_N&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_METADATA_PUSH&, std::unique_ptr<folly::IOBuf>)
      override;
  bool deserializeFrom(Frame_CANCEL&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_RESPONSE&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_ERROR&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_KEEPALIVE&, std::unique_ptr<folly::IOBuf>, bool)
      override;
  bool deserializeFrom(Frame_SETUP&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_LEASE&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_RESUME&, std::unique_ptr<folly::IOBuf>) override;
  bool deserializeFrom(Frame_RESUME_OK&, std::unique_ptr<folly::IOBuf>)
      override;

 private:
  void serializeHeaderInto(
      folly::io::QueueAppender& appender,
      const FrameHeader& header);
  void deserializeHeaderFrom(folly::io::Cursor& cur, FrameHeader& header);

  std::unique_ptr<folly::IOBuf> serializeOutInternal(Frame_REQUEST_Base&&);
  bool deserializeFromInternal(
      Frame_REQUEST_Base&,
      std::unique_ptr<folly::IOBuf>);
};

} // reactivesocket
