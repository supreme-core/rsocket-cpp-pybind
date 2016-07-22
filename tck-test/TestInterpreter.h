// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "TestSuite.h"

namespace reactivesocket {
namespace tck {

class TestInterpreter {
 public:
  explicit TestInterpreter(const Test& test);

  void run();

 private:
  const Test& test_;
};

} // tck
} // reactive socket
