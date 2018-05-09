// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Optional.h>

#include <memory>

#include "rsocket/framing/Frame.h"

namespace rsocket {

// interface separating serialization/deserialization of ReactiveSocket frames
class FrameSerializer {
 public:
  virtual ~FrameSerializer() = default;

  virtual ProtocolVersion protocolVersion() const = 0;

  static std::unique_ptr<FrameSerializer> createFrameSerializer(
      const ProtocolVersion& protocolVersion);

  static std::unique_ptr<FrameSerializer> createAutodetectedSerializer(
      const folly::IOBuf& firstFrame);

  virtual FrameType peekFrameType(const folly::IOBuf& in) const = 0;
  virtual folly::Optional<StreamId> peekStreamId(
      const folly::IOBuf& in) const = 0;

  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_STREAM&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_CHANNEL&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_RESPONSE&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_FNF&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_REQUEST_N&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_METADATA_PUSH&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_CANCEL&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_PAYLOAD&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_ERROR&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_KEEPALIVE&&, bool)
      const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_SETUP&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_LEASE&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(Frame_RESUME&&) const = 0;
  virtual std::unique_ptr<folly::IOBuf> serializeOut(
      Frame_RESUME_OK&&) const = 0;

  virtual bool deserializeFrom(
      Frame_REQUEST_STREAM&,
      std::unique_ptr<folly::IOBuf>) const = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_CHANNEL&,
      std::unique_ptr<folly::IOBuf>) const = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_RESPONSE&,
      std::unique_ptr<folly::IOBuf>) const = 0;
  virtual bool deserializeFrom(
      Frame_REQUEST_FNF&,
      std::unique_ptr<folly::IOBuf>) const = 0;
  virtual bool deserializeFrom(Frame_REQUEST_N&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(
      Frame_METADATA_PUSH&,
      std::unique_ptr<folly::IOBuf>) const = 0;
  virtual bool deserializeFrom(Frame_CANCEL&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(Frame_PAYLOAD&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(Frame_ERROR&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(
      Frame_KEEPALIVE&,
      std::unique_ptr<folly::IOBuf>,
      bool supportsResumability) const = 0;
  virtual bool deserializeFrom(Frame_SETUP&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(Frame_LEASE&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(Frame_RESUME&, std::unique_ptr<folly::IOBuf>)
      const = 0;
  virtual bool deserializeFrom(Frame_RESUME_OK&, std::unique_ptr<folly::IOBuf>)
      const = 0;

  virtual size_t frameLengthFieldSize() const = 0;
  bool& preallocateFrameSizeField();

 protected:
  folly::IOBufQueue createBufferQueue(size_t bufferSize) const;

 private:
  bool preallocateFrameSizeField_{false};
};
} // namespace rsocket
