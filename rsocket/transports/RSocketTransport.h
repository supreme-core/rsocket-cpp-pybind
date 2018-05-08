// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace rsocket {
class RSocketTransportHandler {
 public:
  virtual ~RSocketTransportHandler() = default;

  // connection scope signals
  virtual void onKeepAlive(
      ResumePosition resumePosition,
      std::unique_ptr<folly::IOBuf> data,
      bool keepAliveRespond) = 0;
  virtual void onMetadataPush(std::unique_ptr<folly::IOBuf> metadata) = 0;
  virtual void onResumeOk(ResumePosition resumePosition);
  virtual void onError(ErrorCode errorCode, Payload payload) = 0;

  // stream scope signals
  virtual void onStreamRequestN(StreamId streamId, uint32_t requestN) = 0;
  virtual void onStreamCancel(StreamId streamId) = 0;
  virtual void onStreamError(StreamId streamId, Payload payload) = 0;
  virtual void onStreamPayload(
      StreamId streamId,
      Payload payload,
      bool flagsFollows,
      bool flagsComplete,
      bool flagsNext) = 0;
};

class RSocketTransport {
 public:
  virtual ~RSocketTransport() = default;

  // TODO:
};
} // namespace rsocket
