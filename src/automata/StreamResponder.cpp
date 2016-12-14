// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/automata/StreamResponder.h"
#include "src/Frame.h"

namespace reactivesocket {

void StreamResponder::onNextFrame(Frame_REQUEST_STREAM&& frame) {
  processRequestN(frame);
}
}
