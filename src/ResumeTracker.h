// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/Common.h"

namespace folly {
class IOBuf;
}

namespace reactivesocket {

class ConnectionAutomaton;
class FrameSerializer;

class ResumeTracker {
 public:
  explicit ResumeTracker(ConnectionAutomaton& connection)
      : connection_(connection) {}

  void trackReceivedFrame(const folly::IOBuf& serializedFrame);
  static bool shouldTrackFrame(
      const folly::IOBuf& serializedFrame,
      FrameSerializer& frameSerializer);

  ResumePosition impliedPosition() {
    return impliedPosition_;
  }

  bool canResumeFrom(ResumePosition clientPosition) const {
    return clientPosition <= impliedPosition_;
  }

 private:
  ConnectionAutomaton& connection_;
  ResumePosition impliedPosition_{0};
};
}
