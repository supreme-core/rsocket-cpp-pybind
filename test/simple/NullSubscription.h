// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <stddef.h>
#include "src/ReactiveStreamsCompat.h"
#include "src/mixins/IntrusiveDeleter.h"

namespace reactivesocket {
class NullSubscription : public virtual IntrusiveDeleter, public Subscription {
 public:
  ~NullSubscription() {}

  // Subscription methods
  void request(size_t n) override;

  void cancel() override;
};
}
