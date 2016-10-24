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

folly::Executor& defaultExecutor();

class ExecutorBase {
 public:
  explicit ExecutorBase(
      folly::Executor& executor = defaultExecutor(),
      bool startExecutor = true)
      : executor_(executor) {
    if (!startExecutor) {
      pendingSignals_ = folly::make_unique<PendingSignals>();
    }
  }

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
  std::unique_ptr<PendingSignals> pendingSignals_;
  folly::Executor& executor_;
};

class EnableSharedFromThisVirtualBase
    : public std::enable_shared_from_this<EnableSharedFromThisVirtualBase> {};

template <typename T>
class EnableSharedFromThisBase
    : public virtual EnableSharedFromThisVirtualBase {
 public:
  std::shared_ptr<T> shared_from_this() {
    std::shared_ptr<T> result(
        EnableSharedFromThisVirtualBase::shared_from_this(),
        static_cast<T*>(this));
    return result;
  }

  std::shared_ptr<const T> shared_from_this() const {
    std::shared_ptr<const T> result(
        EnableSharedFromThisVirtualBase::shared_from_this(),
        static_cast<const T*>(this));
    return result;
  }
};

template <typename T>
class SubscriberBaseT : public Subscriber<T>,
                        public EnableSharedFromThisBase<SubscriberBaseT<T>>,
                        public virtual ExecutorBase {
  virtual void onSubscribeImpl(std::shared_ptr<Subscription> subscription) = 0;
  virtual void onNextImpl(T payload) = 0;
  virtual void onCompleteImpl() = 0;
  virtual void onErrorImpl(folly::exception_wrapper ex) = 0;

 public:
  using ExecutorBase::ExecutorBase;

  void onSubscribe(std::shared_ptr<Subscription> subscription) override final {
    runInExecutor(std::bind(
        &SubscriberBaseT::onSubscribeImpl, this->shared_from_this(), subscription));
  }

  void onNext(T payload) override final {
    auto movedPayload = folly::makeMoveWrapper(std::move(payload));
    auto thisPtr = this->shared_from_this();
    // TODO: can we use std::bind and move the parameter into it?
    runInExecutor([thisPtr, movedPayload]() mutable {
      thisPtr->onNextImpl(movedPayload.move());
    });
  }

  void onComplete() override final {
    runInExecutor(
        std::bind(&SubscriberBaseT::onCompleteImpl, this->shared_from_this()));
  }

  void onError(folly::exception_wrapper ex) override final {
    auto movedEx = folly::makeMoveWrapper(std::move(ex));
    auto thisPtr = this->shared_from_this();
    runInExecutor(
        [thisPtr, movedEx]() mutable { thisPtr->onErrorImpl(movedEx.move()); });
  }
};

extern template class SubscriberBaseT<Payload>;
using SubscriberBase = SubscriberBaseT<Payload>;

class SubscriptionBase : public Subscription,
                         public EnableSharedFromThisBase<SubscriptionBase>,
                         public virtual ExecutorBase {
  virtual void requestImpl(size_t n) = 0;
  virtual void cancelImpl() = 0;

 public:
  using ExecutorBase::ExecutorBase;

  void request(size_t n) override final {
    runInExecutor(
        std::bind(&SubscriptionBase::requestImpl, shared_from_this(), n));
  }

  void cancel() override final {
    runInExecutor(std::bind(&SubscriptionBase::cancelImpl, shared_from_this()));
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
    runInExecutor([basePtr, movedPayload]() mutable {
      basePtr->onNext(movedPayload.move());
    });
  }

  void onComplete() {
    runInExecutor(std::bind(&Base::onComplete, this->shared_from_this()));
  }

  void onError(folly::exception_wrapper ex) {
    auto movedEx = folly::makeMoveWrapper(std::move(ex));
    std::shared_ptr<Base> basePtr = this->shared_from_this();
    runInExecutor(
        [basePtr, movedEx]() mutable { basePtr->onError(movedEx.move()); });
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
