// Copyright 2004-present Facebook. All Rights Reserved.

#include "FollyKeepaliveTimer.h"

namespace reactivesocket {

FollyKeepaliveTimer::FollyKeepaliveTimer(
    folly::EventBase& eventBase,
    std::chrono::milliseconds period)
    : eventBase_(eventBase),
      state_(std::make_shared<uint32_t>(0)),
      period_(period) {}

FollyKeepaliveTimer::~FollyKeepaliveTimer() {
  stop();
}

std::chrono::milliseconds FollyKeepaliveTimer::keepaliveTime() {
  return period_;
}

void FollyKeepaliveTimer::schedule() {
  auto scheduledState = state_.get();
  auto currentState = state_;
  eventBase_.runAfterDelay(
      [this, currentState, scheduledState]() {
        LOG(INFO) << "State " << *currentState << " " << scheduledState;

        if (currentState.get() == scheduledState) {
          sendKeepalive();
          schedule();
        }
      },
      static_cast<uint32_t>(keepaliveTime().count()));
}

void FollyKeepaliveTimer::sendKeepalive() {
  if (pending_) {
    connection_->closeWithError(
        Frame_ERROR::connectionError("no response to keepalive"));
    stop();
  } else {
    connection_->sendKeepalive();
    pending_ = true;
  }
}

// must be called from the same thread as start
void FollyKeepaliveTimer::stop() {
  *state_ += 1;
  pending_ = false;
  connection_ = nullptr;
}

// must be called from the same thread as stop
void FollyKeepaliveTimer::start(const std::shared_ptr<FrameSink>& connection) {
  connection_ = connection;
  *state_ += 1;
  DCHECK(!pending_);

  schedule();
}

void FollyKeepaliveTimer::keepaliveReceived() {
  pending_ = false;
}
}
