// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "yarpl/Refcounted.h"

namespace yarpl {
namespace flowable {

class Subscription : public virtual Refcounted {
 public:
  virtual void request(int64_t n) = 0;
  virtual void cancel() = 0;

  static std::shared_ptr<Subscription> empty();
};

} // flowable
} // yarpl
