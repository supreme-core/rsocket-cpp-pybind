// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/FrameSerializer.h"

namespace reactivesocket {

class FrameSerializerV1_0 : public FrameSerializer {
 public:
  constexpr static const ProtocolVersion Version = ProtocolVersion(1, 0);
  ProtocolVersion protocolVersion() override;

  FrameType peekFrameType(const folly::IOBuf& in) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  folly::Optional<StreamId> peekStreamId(const folly::IOBuf& in) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_STREAM&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_CHANNEL&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_RESPONSE&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_FNF&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_REQUEST_N&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_METADATA_PUSH&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESPONSE&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_KEEPALIVE&&, bool) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME_OK&&) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_REQUEST_STREAM&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_REQUEST_CHANNEL&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_REQUEST_RESPONSE&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_REQUEST_FNF&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_REQUEST_N&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_METADATA_PUSH&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_CANCEL&, std::unique_ptr<folly::IOBuf>) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_RESPONSE&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_ERROR&, std::unique_ptr<folly::IOBuf>) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_KEEPALIVE&, std::unique_ptr<folly::IOBuf>, bool)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_SETUP&, std::unique_ptr<folly::IOBuf>) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_LEASE&, std::unique_ptr<folly::IOBuf>) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_RESUME&, std::unique_ptr<folly::IOBuf>) override {
    throw std::runtime_error("v1 serialization not implemented");
  }

  bool deserializeFrom(Frame_RESUME_OK&, std::unique_ptr<folly::IOBuf>)
      override {
    throw std::runtime_error("v1 serialization not implemented");
  }
};
} // reactivesocket
