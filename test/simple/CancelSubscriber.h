// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include "src/ReactiveStreamsCompat.h"
#include "src/Payload.h"
#include "src/mixins/IntrusiveDeleter.h"

namespace reactivesocket {
class CancelSubscriber : public IntrusiveDeleter, public Subscriber<Payload> {
 public:
  ~CancelSubscriber() override = default;

  void onSubscribe(Subscription& subscription) override;

  void onNext(Payload element) override;

  void onComplete() override;

  void onError(folly::exception_wrapper ex) override;
};
}
