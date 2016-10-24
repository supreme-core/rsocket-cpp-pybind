// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <queue>

#include <folly/ExceptionWrapper.h>
#include <folly/Executor.h>
#include <folly/Memory.h>
#include <folly/MoveWrapper.h>
#include <folly/io/IOBuf.h>

#include "src/ConnectionAutomaton.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"

namespace reactivesocket {

class ExecutorBase {
public:
    ExecutorBase(        folly::Executor& executor)
        : executor_(executor) {}

/// We start in a queueing mode, where it merely queues signal
  /// deliveries until ::start is invoked.
  ///
  /// Calling into this method may deliver all enqueued signals immediately.
  void start() {
    if (pendingSignals_) {
      auto movedSignals = folly::makeMoveWrapper(std::move(pendingSignals_));
      if (!(*movedSignals)->empty()) {
        runInExecutor([movedSignals]() mutable {
          for (auto& signal : **movedSignals) {
            signal();
          }
        });
      }
    }
  }

 protected:
  template <typename F>
  void runInExecutor(F&& func) {
    if (pendingSignals_) {
      pendingSignals_->emplace_back(func);
    } else {
      executor_.add(std::move(func));
    }
  }

private:
  using PendingSignals = std::vector<std::function<void()>>;
  std::unique_ptr<PendingSignals> pendingSignals_{
      folly::make_unique<PendingSignals>()};

    folly::Executor& executor_;

};

//TODO: look into how to virtually inherit std::enable_shared_from_this
class SharedFromThisBase {
  virtual std::shared_ptr<SharedFromThisBase> sharedFromThisImpl() = 0;
 protected:
  template<typename T>
  std::shared_ptr<T> sharedFromThis() {
    return std::static_pointer_cast<T>(sharedFromThisImpl());
  }
};

class SubscriberBase : public Subscriber<Payload>,
                       public SharedFromThisBase,
                       public virtual ExecutorBase {
  virtual void onSubscribeImpl(std::shared_ptr<Subscription> subscription) = 0;
  virtual void onNextImpl(Payload payload) = 0;
  virtual void onCompleteImpl() = 0;
  virtual void onErrorImpl(folly::exception_wrapper ex) = 0;

public:
  using ExecutorBase::ExecutorBase;

  void onSubscribe(std::shared_ptr<Subscription> subscription) override final {
    runInExecutor(std::bind(&SubscriberBase::onSubscribeImpl, sharedFromThis<SubscriberBase>(), subscription));
  }

  void onNext(Payload payload) override final {
    auto movedPayload = folly::makeMoveWrapper(std::move(payload));
    auto thisPtr = sharedFromThis<SubscriberBase>();
    //TODO: can we use std::bind and move the parameter into it?
    runInExecutor(
        [thisPtr, movedPayload]() mutable { thisPtr->onNextImpl(movedPayload.move()); });
  }

  void onComplete() override final {
    runInExecutor(std::bind(&SubscriberBase::onCompleteImpl, sharedFromThis<SubscriberBase>()));
  }

  void onError(folly::exception_wrapper ex) override final {
    auto movedEx = folly::makeMoveWrapper(std::move(ex));
    auto thisPtr = sharedFromThis<SubscriberBase>();
    runInExecutor([thisPtr, movedEx]() mutable { thisPtr->onErrorImpl(movedEx.move()); });
  }
};

class SubscriptionBase : public Subscription,
                         public SharedFromThisBase,
                         public virtual ExecutorBase {
  virtual void requestImpl(size_t n) = 0;
  virtual void cancelImpl() = 0;

public:
  using ExecutorBase::ExecutorBase;

  void request(size_t n) override final {
    runInExecutor(std::bind(&SubscriptionBase::requestImpl, sharedFromThis<SubscriptionBase>(), n));
  }

  void cancel() override final {
    runInExecutor(std::bind(&SubscriptionBase::cancelImpl, sharedFromThis<SubscriptionBase>()));
  }
};



/// Instead of calling into the respective Base methods, schedules signals
/// delivery on an executor. Non-signal methods are simply forwarded.
///
/// Uses lazy method instantiantiation trick, see LoggingMixin.
template <typename Base>
class ExecutorMixin : public Base,
                      public std::enable_shared_from_this<ExecutorMixin<Base>> {
  static_assert(
      !std::is_base_of<std::enable_shared_from_this<Base>, Base>::value,
      "enable_shared_from_this is already inherited");

 public:
  struct Parameters : Base::Parameters {
    Parameters(
        const typename Base::Parameters& baseParams,
        folly::Executor& _executor)
        : Base::Parameters(baseParams), executor(_executor) {}
    folly::Executor& executor;
  };

  ExecutorMixin(const Parameters& params)
      : Base(params), executor_(params.executor) {}
  ~ExecutorMixin() {}

  /// The mixin starts in a queueing mode, where it merely queues signal
  /// deliveries until ::start is invoked.
  ///
  /// Calling into this method may deliver all enqueued signals immediately.
  void start() {
    if (pendingSignals_) {
      auto movedSignals = folly::makeMoveWrapper(std::move(pendingSignals_));
      if (!(*movedSignals)->empty()) {
        runInExecutor([movedSignals]() mutable {
          for (auto& signal : **movedSignals) {
            signal();
          }
        });
      }
    }
  }

  /// @{
  /// Publisher<Payload>
  void subscribe(std::shared_ptr<Subscriber<Payload>> subscriber) {
    // This call punches through the executor-enforced ordering, to ensure that
    // the Subscriber pointer is set as soon as possible.
    // More esoteric reason: this is not a signal in ReactiveStreams language.
    Base::subscribe(std::move(subscriber));
  }
  /// @}

  /// @{
  /// Subscription
  void request(size_t n) {
    runInExecutor(std::bind(&Base::request, this->shared_from_this(), n));
  }

  void cancel() {
    runInExecutor(std::bind(&Base::cancel, this->shared_from_this()));
  }
  /// @}

  /// @{
  /// Subscriber<Payload>
  void onSubscribe(std::shared_ptr<Subscription> subscription) {
    // This call punches through the executor-enforced ordering, to ensure that
    // the Subscription pointer is set as soon as possible.
    // More esoteric reason: this is not a signal in ReactiveStreams language.
    Base::onSubscribe(std::move(subscription));
  }

  void onNext(Payload payload) {
    auto movedPayload = folly::makeMoveWrapper(std::move(payload));
    std::shared_ptr<Base> basePtr = this->shared_from_this();
    runInExecutor(
        [basePtr, movedPayload]() mutable { basePtr->onNext(movedPayload.move()); });
  }

  void onComplete() {
    runInExecutor(std::bind(&Base::onComplete, this->shared_from_this()));
  }

  void onError(folly::exception_wrapper ex) {
    auto movedEx = folly::makeMoveWrapper(std::move(ex));
    std::shared_ptr<Base> basePtr = this->shared_from_this();
    runInExecutor([basePtr, movedEx]() mutable { basePtr->onError(movedEx.move()); });
  }
  /// @}

  /// @{
  void endStream(StreamCompletionSignal signal) {
    Base::endStream(signal);
  }
  /// @}

  std::ostream& logPrefix(std::ostream& os) {
    return os << "ExecutorMixin(" << &this->connection_ << ", "
              << this->streamId_ << "): ";
  }

 protected:
  /// @{
  template <typename Frame>
  void onNextFrame(Frame&& frame) {
    Base::onNextFrame(std::move(frame));
  }

  void onBadFrame() {
    Base::onBadFrame();
  }
  /// @}

 private:
  template <typename F>
  void runInExecutor(F&& func) {
    if (pendingSignals_) {
      pendingSignals_->emplace_back(func);
    } else {
      executor_.add(std::move(func));
    }
  }

  folly::Executor& executor_;
  using PendingSignals = std::vector<std::function<void()>>;
  std::unique_ptr<PendingSignals> pendingSignals_{
      folly::make_unique<PendingSignals>()};
};
}
