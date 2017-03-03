// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <functional>
#include <memory>

namespace yarpl {
namespace flowable {

class Subscription {
 public:
  virtual ~Subscription() = default;

  virtual void cancel() = 0;
  virtual void request(uint64_t n) = 0;
};

} // observable namespace
} // yarpl namespace
