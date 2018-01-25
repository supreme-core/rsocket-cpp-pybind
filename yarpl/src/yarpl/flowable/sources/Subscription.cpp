// Copyright 2004-present Facebook. All Rights Reserved.

#include "yarpl/flowable/Subscription.h"

namespace yarpl {
namespace flowable {

std::shared_ptr<Subscription> Subscription::empty() {
  class NullSubscription : public Subscription {
    void request(int64_t) override {}
    void cancel() override {}
  };
  return std::make_shared<NullSubscription>();
}

} // flowable
} // yarpl
