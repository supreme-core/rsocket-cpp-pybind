// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace reactivesocket {
class Stats {
 public:
  static Stats& noop();

  virtual void socketCreated() = 0;
  virtual void socketClosed() = 0;
  virtual void connectionCreated(const std::string& type) = 0;
  virtual void connectionClosed(const std::string& type) = 0;

  virtual ~Stats() = default;
};
}
