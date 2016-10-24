// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBufQueue.h>
#include <reactive-streams/utilities/AllowanceSemaphore.h>
#include <reactive-streams/utilities/SmartPointers.h>
#include "src/ReactiveStreamsCompat.h"
#include "src/mixins/ExecutorMixin.h"

namespace reactivesocket {

class FramedReader : public SubscriberBaseT<std::unique_ptr<folly::IOBuf>>,
                     public SubscriptionBase,
                     public EnableSharedFromThisBase<FramedReader> {
 public:
  FramedReader(
      std::shared_ptr<reactivesocket::Subscriber<std::unique_ptr<folly::IOBuf>>>
          frames)
      : frames_(std::move(frames)),
        payloadQueue_(folly::IOBufQueue::cacheChainLength()) {}

  //
  // public methods should be guarded by the DestructorGuard
  // if there is code to execute after calling into the callbacks
  //

 private:

  using EnableSharedFromThisBase<FramedReader>::shared_from_this;

  // Subscriber methods
  void onSubscribeImpl(std::shared_ptr<Subscription> subscription) override;
  void onNextImpl(std::unique_ptr<folly::IOBuf> element) override;
  void onCompleteImpl() override;
  void onErrorImpl(folly::exception_wrapper ex) override;

  // Subscription methods
  void requestImpl(size_t n) override;
  void cancelImpl() override;

  void parseFrames();
  void requestStream();

  SubscriberPtr<reactivesocket::Subscriber<std::unique_ptr<folly::IOBuf>>>
      frames_;
  SubscriptionPtr<Subscription> streamSubscription_;

  ::reactivestreams::AllowanceSemaphore allowance_{0};

  bool streamRequested_{false};
  bool dispatchingFrames_{false};

  folly::IOBufQueue payloadQueue_;
};

} // reactive socket
