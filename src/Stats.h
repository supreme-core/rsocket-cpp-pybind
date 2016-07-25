// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace reactivesocket {
// forward decl for NoopStats
class NoopStats;

class Stats {
 public:
  static NoopStats noop;

  virtual void socketCreated() = 0;
  virtual void socketClosed() = 0;
  virtual ~Stats() = default;
};

class NoopStats : public Stats {
 public:
  void socketCreated() override {};
  void socketClosed() override {};
};
}
