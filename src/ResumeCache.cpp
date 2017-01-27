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
    case FrameType::REQUEST_RESPONSE:
    case FrameType::REQUEST_FNF:
    case FrameType::REQUEST_N:
    case FrameType::CANCEL:
    case FrameType::ERROR:
    case FrameType::RESPONSE:

      addFrame(serializedFrame);
      // TODO(tmont): this could be expensive, find a better way to determine
      // frame length
      position_ += serializedFrame.computeChainDataLength();

      // TODO(tmont): this is not ideal, but memory usage is more important
      if (streamIdPtr) {
        const StreamId streamId = *streamIdPtr;

        streamMap_[streamId] = position_;
      }

      break;

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

void ResumeCache::resetUpToPosition(ResumePosition position) {
  if (position <= resetPosition_) {
    return;
  }

  if (position > position_) {
    position = position_;
  }

  for (auto it = streamMap_.begin(); it != streamMap_.end();) {
    if (it->second <= position) {
      it = streamMap_.erase(it);
    } else {
      it++;
    }
  }

  if (!frames_.empty()) {
    auto end = std::lower_bound(
        frames_.begin(),
        frames_.end(),
        position,
        [](decltype(frames_.back())& pair, ResumePosition pos) {
          return pair.first < pos;
        });

    if (end == frames_.end()) {
      stats_.resumeBufferChanged(
          -static_cast<int>(frames_.size()),
          static_cast<int>(
              static_cast<int64_t>(position_) -
              static_cast<int64_t>(resetPosition_)));
      frames_.clear();
    } else {
      stats_.resumeBufferChanged(
          -static_cast<int>(std::distance(frames_.begin(), end)),
          static_cast<int>(
              static_cast<int64_t>(end->first) -
              static_cast<int64_t>(resetPosition_)));
      frames_.erase(frames_.begin(), end);
    }
  }

  resetPosition_ = position;
  DCHECK(frames_.empty() || frames_.front().first == resetPosition_);
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

  if (position == position_) {
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

  while (found != frames_.end()) {
    frameTransport.outputFrameOrEnqueue(found->second->clone());
    found++;
  }
}

} // reactivesocket
