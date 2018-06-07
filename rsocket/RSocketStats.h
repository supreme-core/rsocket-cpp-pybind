// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Optional.h>
#include <cstdint>
#include <memory>
#include <string>

#include "rsocket/framing/FrameType.h"
#include "rsocket/internal/Common.h"

namespace rsocket {

class DuplexConnection;

class RSocketStats {
 public:
  enum class ResumeOutcome { SUCCESS, FAILURE };

  virtual ~RSocketStats() = default;

  static std::shared_ptr<RSocketStats> noop();

  virtual void socketCreated() {}
  virtual void socketConnected() {}
  virtual void socketDisconnected() {}
  virtual void socketClosed(StreamCompletionSignal /* signal */) {}

  virtual void serverConnectionAccepted() {}

  virtual void duplexConnectionCreated(
      const std::string& /* type */,
      DuplexConnection* /* connection */) {}
  virtual void duplexConnectionClosed(
      const std::string& /* type */,
      DuplexConnection* /* connection */) {}
  virtual void serverResume(
      folly::Optional<int64_t> /* clientAvailable */,
      int64_t /* serverAvailable */,
      int64_t /* serverDelta */,
      ResumeOutcome /* outcome */) {}
  virtual void bytesWritten(size_t /* bytes */) {}
  virtual void bytesRead(size_t /* bytes */) {}
  virtual void frameWritten(FrameType /* frameType */) {}
  virtual void frameRead(FrameType /* frameType */) {}
  virtual void resumeBufferChanged(
      int /* framesCountDelta */,
      int /* dataSizeDelta */) {}
  virtual void streamBufferChanged(
      int64_t /* framesCountDelta */,
      int64_t /* dataSizeDelta */) {}
  virtual void resumeFailedNoState() {}
  virtual void keepaliveSent() {}
  virtual void keepaliveReceived() {}
  virtual void unknownFrameReceived() {
  } // TODO(lehecka): add to all implementations
};
} // namespace rsocket
