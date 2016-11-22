// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <folly/Optional.h>

#include "src/Frame.h"

namespace reactivesocket {

class ConnectionAutomaton;

class ResumeCache {
 public:
  using position_t = ResumePosition;

  ResumeCache() : position_(0), resetPosition_(0) {}

  void trackSentFrame(const folly::IOBuf& serializedFrame);

  // called to clear up to a certain position from the cache (from keepalive or resuming)
  void resetUpToPosition(const position_t position) {
      for (auto it = streamMap_.begin(); it != streamMap_.end();) {
          if (it->second <= position) {
              it = streamMap_.erase(it);
          } else {
              it++;
          }
      }
      resetPosition_ = position;
  }

  bool isPositionAvailable(position_t position) {
    return (position == position_);
  }

  bool isPositionAvailable(position_t position, StreamId streamId)
  {
    bool result = false;

    auto it = streamMap_.find(streamId);
    if (it != streamMap_.end()) {
      const position_t streamPosition = (*it).second;

      result = (streamPosition <= position);
    } else {
      result = (resetPosition_ >= position);
    }

    return result;
  }

  position_t position() {
    return position_;
  }

 private:
  position_t position_;
  position_t resetPosition_;
  std::unordered_map<StreamId, position_t> streamMap_;
};
}
