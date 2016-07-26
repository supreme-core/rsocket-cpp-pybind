// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <src/Stats.h>

namespace reactivesocket {
class StatsPrinter : public Stats {
 public:
  virtual ~StatsPrinter() = default;

  void socketCreated() override;

  void socketClosed() override;

    virtual void connectionCreated(const std::string &type) override;

    virtual void connectionClosed(const std::string &type) override;
};
}