// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/ResumeCache.h"
#include <folly/Optional.h>
#include <algorithm>
#include "src/FrameTransport.h"
#include "src/Stats.h"

namespace reactivesocket {

void ResumeCache::trackSentFrame(const folly::IOBuf& serializedFrame) {
  auto frameType = FrameHeader::peekType(serializedFrame);
  auto streamIdPtr = FrameHeader::peekStreamId(serializedFrame);

  switch (frameType) {
    case FrameType::REQUEST_CHANNEL:
    case FrameType::REQUEST_STREAM:
    case FrameType::REQUEST_SUB:
    case FrameType::REQUEST_RESPONSE:
    case FrameType::REQUEST_FNF:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::ERROR:
    case FrameType::RESPONSE: {
      // TODO(tmont): this could be expensive, find a better way to determine
      // frame length
      auto frameDataLength = serializedFrame.computeChainDataLength();
      position_ += frameDataLength;

      // TODO(tmont): this is not ideal, but memory usage is more important
      if (streamIdPtr) {
        const StreamId streamId = *streamIdPtr;

        streamMap_[streamId] = position_;
      }

      addFrame(serializedFrame, frameDataLength);
    } break;

    case FrameType::RESERVED:
    case FrameType::SETUP:
    case FrameType::LEASE:
    case FrameType::KEEPALIVE:
    case FrameType::METADATA_PUSH:
    case FrameType::RESUME:
    case FrameType::RESUME_OK:
    default:
      break;
  }
}

void ResumeCache::resetUpToPosition(const ResumePosition position) {
  if (resetPosition_ == position) {
    return;
  }

  for (auto it = streamMap_.begin(); it != streamMap_.end();) {
    if (it->second <= position) {
      it = streamMap_.erase(it);
    } else {
      it++;
    }
  }

  resetPosition_ = position;

  auto end = std::upper_bound(
      frames_.begin(),
      frames_.end(),
      position,
      [](ResumePosition pos, decltype(frames_.back())& pair) {
        return pos < pair.first;
      });

  int dataSize = 0;
  int framesCount = 0;
  for (auto begin = frames_.begin(); begin != end; ++begin) {
    dataSize += begin->second->computeChainDataLength();
    ++framesCount;
  }

  frames_.erase(frames_.begin(), end);
  stats_.resumeBufferChanged(-framesCount, -dataSize);
}

bool ResumeCache::isPositionAvailable(ResumePosition position) const {
  return (position_ == position) || (resetPosition_ == position) ||
      std::binary_search(
             frames_.begin(),
             frames_.end(),
             std::make_pair(position, std::unique_ptr<folly::IOBuf>()),
             [](decltype(frames_.back())& pairA,
                decltype(frames_.back())& pairB) {
               return pairA.first < pairB.first;
             });
}

bool ResumeCache::isPositionAvailable(
    ResumePosition position,
    StreamId streamId) const {
  bool result = false;

  auto it = streamMap_.find(streamId);
  if (it != streamMap_.end()) {
    const ResumePosition streamPosition = (*it).second;

    result = (streamPosition <= position);
  } else {
    result = (resetPosition_ >= position);
  }

  return result;
}

void ResumeCache::addFrame(const folly::IOBuf& frame, size_t frameDataLength) {
  // TODO: implement bounds to the buffer
  frames_.emplace_back(position_, frame.clone());
  stats_.resumeBufferChanged(1, static_cast<int>(frameDataLength));
}

void ResumeCache::sendFramesFromPosition(
    ResumePosition position,
    FrameTransport& frameTransport) const {
  DCHECK(isPositionAvailable(position));

  if (position == resetPosition_) {
    // idle resumption
    return;
  }

  auto found = std::lower_bound(
      frames_.begin(),
      frames_.end(),
      position,
      [](decltype(frames_.back())& pair, ResumePosition pos) {
        return pair.first < pos;
      });

  DCHECK(found != frames_.end());
  DCHECK(found->first == position);

  while (++found != frames_.end()) {
    frameTransport.outputFrameOrEnqueue(found->second->clone());
  }
}

} // reactivesocket
