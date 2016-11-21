// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <stdexcept>

//
// this file includes all PUBLIC common types.
//

namespace reactivesocket {

class StreamInterruptedException : public std::runtime_error {
 public:
  explicit StreamInterruptedException(int _terminatingSignal);
  int terminatingSignal;
};
}
