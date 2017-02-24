// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/ResumeCache.h"
#include <folly/Optional.h>
#include <algorithm>
#include "src/ConnectionAutomaton.h"
#include "src/FrameTransport.h"
#include "src/ResumeTracker.h"
#include "src/Stats.h"

namespace reactivesocket {

ResumeCache::~ResumeCache() {
  if (!frames_.empty()) {
    onClearFrames();
  }
}

void ResumeCache::trackSentFrame(const folly::IOBuf& serializedFrame) {
  if (ResumeTracker::shouldTrackFrame(
          serializedFrame, connection_.frameSerializer())) {
    // TODO(tmont): this could be expensive, find a better way to determine
    auto frameDataLength = serializedFrame.computeChainDataLength();
    addFrame(serializedFrame, frameDataLength);

    position_ += frameDataLength;

    // TODO(tmont): this is not ideal, but memory usage is more important
    auto streamIdPtr =
        connection_.frameSerializer().peekStreamId(serializedFrame);
    if (streamIdPtr) {
      const StreamId streamId = *streamIdPtr;

      streamMap_[streamId] = position_;
    }
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
      onClearFrames();
      frames_.clear();
    } else {
      DCHECK(end->first >= resetPosition_);
      connection_.stats().resumeBufferChanged(
          -static_cast<int>(std::distance(frames_.begin(), end)),
          -static_cast<int>(end->first - resetPosition_));
      frames_.erase(frames_.begin(), end);
    }
  }

  resetPosition_ = position;
  DCHECK(frames_.empty() || frames_.front().first == resetPosition_);
}

bool ResumeCache::isPositionAvailable(ResumePosition position) const {
  return (position_ == position) ||
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
  connection_.stats().resumeBufferChanged(1, static_cast<int>(frameDataLength));
}

void ResumeCache::onClearFrames() {
  DCHECK(position_ >= resetPosition_);
  DCHECK(!frames_.empty());
  connection_.stats().resumeBufferChanged(
      -static_cast<int>(frames_.size()),
      -static_cast<int>(position_ - resetPosition_));
}

void ResumeCache::sendFramesFromPosition(
    ResumePosition position,
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
      [](decltype(frames_.back())& pair, ResumePosition pos) {
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
