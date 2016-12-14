// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/automata/StreamAutomatonBase.h"
#include "src/ConnectionAutomaton.h"
#include "src/Frame.h"

namespace reactivesocket {

void StreamAutomatonBase::endStream(StreamCompletionSignal) {
  isTerminated_ = true;
}

void StreamAutomatonBase::onNextFrame(Frame_REQUEST_STREAM&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_REQUEST_SUB&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_REQUEST_CHANNEL&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_REQUEST_RESPONSE&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_REQUEST_N&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_CANCEL&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_RESPONSE&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onNextFrame(Frame_ERROR&&) {
  onUnexpectedFrame();
}

void StreamAutomatonBase::onBadFrame() {
  connection_->disconnectWithError(
      Frame_ERROR::invalid(streamId_, "bad frame"));
}

void StreamAutomatonBase::onUnexpectedFrame() {
  DCHECK(false) << "onUnexpectedFrame";
  connection_->disconnectWithError(Frame_ERROR::unexpectedFrame());
}
} // reactivesocket