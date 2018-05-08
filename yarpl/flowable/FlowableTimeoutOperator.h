// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace yarpl {
class TimeoutException : public std::runtime_error {
 public:
  TimeoutException() : std::runtime_error("yarpl::TimeoutException") {}
};
namespace detail {
class TimeoutExceptionGenerator {
 public:
  TimeoutException operator()() const {
    return {};
  }
};
} // namespace detail

namespace flowable {
namespace details {

template <typename T, typename ExceptionGenerator>
class TimeoutOperator : public FlowableOperator<T, T> {
  using Super = FlowableOperator<T, T>;

 public:
  TimeoutOperator(
      std::shared_ptr<Flowable<T>> upstream,
      folly::EventBase& timerEvb,
      std::chrono::milliseconds timeout,
      std::chrono::milliseconds initTimeout,
      ExceptionGenerator&& exnGen)
      : upstream_(std::move(upstream)),
        timerEvb_(timerEvb),
        timeout_(timeout),
        initTimeout_(initTimeout),
        exnGen_(std::forward<ExceptionGenerator>(exnGen)) {}

  void subscribe(std::shared_ptr<Subscriber<T>> subscriber) override {
    auto subscription = std::make_shared<TimeoutSubscription>(
        this->ref_from_this(this),
        subscriber,
        timerEvb_,
        initTimeout_,
        timeout_);
    upstream_->subscribe(std::move(subscription));
  }

 protected:
  class TimeoutSubscription : public Super::Subscription,
                              public folly::HHWheelTimer::Callback {
    using SuperSub = typename Super::Subscription;

   public:
    TimeoutSubscription(
        std::shared_ptr<TimeoutOperator<T, ExceptionGenerator>> flowable,
        std::shared_ptr<Subscriber<T>> subscriber,
        folly::EventBase& timerEvb,
        std::chrono::milliseconds initTimeout,
        std::chrono::milliseconds timeout)
        : Super::Subscription(std::move(subscriber)),
          flowable_(std::move(flowable)),
          timerEvb_(timerEvb),
          initTimeout_(initTimeout),
          timeout_(timeout) {}

    void onSubscribeImpl() override {
      DCHECK(timerEvb_.isInEventBaseThread());
      if (initTimeout_.count() > 0) {
        nextTime_ = getCurTime() + initTimeout_;
        timerEvb_.timer().scheduleTimeout(this, initTimeout_);
      } else {
        nextTime_ = std::chrono::steady_clock::time_point::max();
      }

      SuperSub::onSubscribeImpl();
    }

    void onNextImpl(T value) override {
      DCHECK(timerEvb_.isInEventBaseThread());
      if (flowable_) {
        if (nextTime_ != std::chrono::steady_clock::time_point::max()) {
          cancelTimeout(); // cancel timer before calling onNext
          auto currentTime = getCurTime();
          if (currentTime > nextTime_) {
            timeoutExpired();
            return;
          }
          nextTime_ = std::chrono::steady_clock::time_point::max();
        }

        SuperSub::subscriberOnNext(std::move(value));

        if (timeout_.count() > 0) {
          nextTime_ = getCurTime() + timeout_;
          timerEvb_.timer().scheduleTimeout(this, timeout_);
        }
      }
    }

    void onTerminateImpl() override {
      DCHECK(timerEvb_.isInEventBaseThread());
      flowable_.reset();
      cancelTimeout();
    }

    void timeoutExpired() noexcept override {
      if (auto flowable = std::exchange(flowable_, nullptr)) {
        SuperSub::terminateErr([&]() -> folly::exception_wrapper {
          try {
            return flowable->exnGen_();
          } catch (...) {
            return folly::make_exception_wrapper<std::nested_exception>();
          }
        }());
      }
    }

    void callbackCanceled() noexcept override {
      // Do nothing..
    }

   private:
    std::shared_ptr<TimeoutOperator<T, ExceptionGenerator>> flowable_;
    folly::EventBase& timerEvb_;
    std::chrono::milliseconds initTimeout_;
    std::chrono::milliseconds timeout_;
    std::chrono::steady_clock::time_point nextTime_;
  };

  std::shared_ptr<Flowable<T>> upstream_;
  folly::EventBase& timerEvb_;
  std::chrono::milliseconds timeout_;
  std::chrono::milliseconds initTimeout_;
  std::decay_t<ExceptionGenerator> exnGen_;
};

} // namespace details
} // namespace flowable
} // namespace yarpl
