// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/Stats.h"

namespace reactivesocket {

class NoopStats : public Stats {
 public:
  void socketCreated() override{};
  void socketClosed() override{};
  void connectionCreated(const std::string& type) override{};
  void connectionClosed(const std::string& type) override{};
};

Stats& Stats::noop() {
  static NoopStats noop_;
  return noop_;
};
}
