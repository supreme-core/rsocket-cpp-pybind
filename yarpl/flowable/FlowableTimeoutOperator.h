// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace yarpl {
namespace flowable {
namespace details {

template <typename T>
class TimeoutOperator : public FlowableOperator<T, T> {
  using Super = FlowableOperator<T, T>;

 public:
  TimeoutOperator(
      std::shared_ptr<Flowable<T>> upstream,
      folly::EventBase& timerEvb,
      std::chrono::milliseconds timeout,
      std::chrono::milliseconds initTimeout = std::chrono::milliseconds(0))
      : upstream_(std::move(upstream)),
        timerEvb_(timerEvb),
        timeout_(timeout),
        initTimeout_(initTimeout) {}

  void subscribe(std::shared_ptr<Subscriber<T>> subscriber) override {
    auto subscription = std::make_shared<TimeoutSubscription>(
        subscriber, timerEvb_, initTimeout_, timeout_);
    upstream_->subscribe(std::move(subscription));
  }

 protected:
  class TimeoutSubscription : public Super::Subscription,
                              public folly::HHWheelTimer::Callback {
    using SuperSub = typename Super::Subscription;

   public:
    TimeoutSubscription(
        std::shared_ptr<Subscriber<T>> subscriber,
        folly::EventBase& timerEvb,
        std::chrono::milliseconds initTimeout,
        std::chrono::milliseconds timeout)
        : Super::Subscription(std::move(subscriber)),
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
      if (!canceled_) {
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
      canceled_ = true;
      cancelTimeout();
    }

    void timeoutExpired() noexcept override {
      if (!canceled_) {
        canceled_ = true;
        SuperSub::terminateErr(std::runtime_error("timeout expired"));
      }
    }

    void callbackCanceled() noexcept override {
      // Do nothing..
    }

   private:
    folly::EventBase& timerEvb_;
    std::chrono::milliseconds initTimeout_;
    std::chrono::milliseconds timeout_;
    bool canceled_{false};
    std::chrono::steady_clock::time_point nextTime_;
  };

  std::shared_ptr<Flowable<T>> upstream_;
  folly::EventBase& timerEvb_;
  std::chrono::milliseconds timeout_;
  std::chrono::milliseconds initTimeout_;
};

} // namespace details
} // namespace flowable
} // namespace yarpl
