// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace reactivesocket {
// forward decl for NoopStats
class NoopStats;

// Virtual interface for a stats receiver
class Stats {
 public:
  static NoopStats noop;

  virtual void socketCreated() = 0;
  virtual void socketClosed() = 0;
  virtual ~Stats() = default;
};

class NoopStats : public Stats {
 public:
  virtual ~NoopStats() = default;

  void socketCreated(){};

  void socketClosed(){};
};
}
