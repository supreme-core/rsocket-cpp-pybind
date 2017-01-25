// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/ResumeCache.h"
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

      addFrame(serializedFrame);
      break;

    case FrameType::RESERVED:
    case FrameType::SETUP:
    case FrameType::LEASE:
    case FrameType::KEEPALIVE:
    case FrameType::REQUEST_RESPONSE: // TODO: fix the bug
    case FrameType::REQUEST_FNF:
    case FrameType::METADATA_PUSH:
    case FrameType::RESUME:
    case FrameType::RESUME_OK:
    default:
      break;
  }
}

void ResumeCache::resetUpToPosition(const position_t position) {
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
      [](position_t pos, decltype(frames_.back())& pair) {
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

bool ResumeCache::isPositionAvailable(position_t position) const {
  return (position == position_) ||
      std::binary_search(
             frames_.begin(),
             frames_.end(),
             std::make_pair(position, std::unique_ptr<folly::IOBuf>()),
             [](decltype(frames_.back())& pairA,
                decltype(frames_.back())& pairB) {
               return pairA.first < pairB.first;
             });
}

bool ResumeCache::isPositionAvailable(position_t position, StreamId streamId)
    const {
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

void ResumeCache::addFrame(const folly::IOBuf& frame) {
  // TODO: implement bounds to the buffer
  frames_.emplace_back(position_, frame.clone());
  stats_.resumeBufferChanged(
      1, static_cast<int>(frame.computeChainDataLength()));
}

void ResumeCache::sendFramesFromPosition(
    position_t position,
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
      [](decltype(frames_.back())& pair, position_t pos) {
        return pair.first < pos;
      });

  DCHECK(found != frames_.end());
  DCHECK(found->first == position);

  while (++found != frames_.end()) {
    frameTransport.outputFrameOrEnqueue(found->second->clone());
  }
}

} // reactivesocket
