// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>

#include "src/ResumeCache.h"
#include "src/ResumeTracker.h"

namespace folly {
class IOBuf;
}

namespace reactivesocket {

class ConnectionAutomaton;
class StreamAutomatonBase;
using StreamId = uint32_t;

class StreamState {
 public:
  explicit StreamState(ConnectionAutomaton& connection)
      : resumeTracker_(connection), resumeCache_(connection) {}

  void enqueueOutputPendingFrame(std::unique_ptr<folly::IOBuf> frame) {
    outputFrames_.push_back(std::move(frame));
  }

  std::deque<std::unique_ptr<folly::IOBuf>> moveOutputPendingFrames() {
    return std::move(outputFrames_);
  }

  std::unordered_map<StreamId, std::shared_ptr<StreamAutomatonBase>> streams_;
  ResumeTracker resumeTracker_;
  ResumeCache resumeCache_;

 private:
  std::deque<std::unique_ptr<folly::IOBuf>> outputFrames_;
};
}
