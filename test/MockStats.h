// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

#include <gmock/gmock.h>
#include <src/Stats.h>

#include "src/Payload.h"

namespace reactivesocket {

class MockStats : public Stats {
 public:
  MOCK_METHOD0(socketCreated_, void());
  MOCK_METHOD0(socketClosed_, void());

  void socketCreated() override {
    socketCreated_();
  }

  void socketClosed() override {
    socketClosed_();
  }
};
}
