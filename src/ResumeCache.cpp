// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/ResumeCache.h"

namespace reactivesocket {

void ResumeCache::trackSentFrame(const folly::IOBuf& serializedFrame) {
  auto frameType = FrameHeader::peekType(serializedFrame);
  auto streamIdPtr = FrameHeader::peekStreamId(serializedFrame);

  switch (frameType) {
    case FrameType::REQUEST_CHANNEL:
    case FrameType::REQUEST_STREAM:
    case FrameType::REQUEST_SUB:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::ERROR:
    case FrameType::RESPONSE:
      // TODO(tmont): this could be expensive, find a better way to determine
      // frame length
      position_ += serializedFrame.computeChainDataLength();

      // TODO(tmont): this is not ideal, but memory usage is more important
      if (streamIdPtr) {
        const StreamId streamId = *streamIdPtr;

        streamMap_[streamId] = position_;
      }
      break;

    default:
      break;
  }
}
}
