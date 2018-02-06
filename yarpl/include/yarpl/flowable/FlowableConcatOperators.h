// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace yarpl {
namespace flowable {
namespace details {

template <typename T>
class ConcatWithOperator : public FlowableOperator<T, T> {
  using Super = FlowableOperator<T, T>;

 public:
  ConcatWithOperator(
      std::shared_ptr<Flowable<T>> first,
      std::shared_ptr<Flowable<T>> second)
      : first_(std::move(first)), second_(std::move(second)) {
    CHECK(first_);
    CHECK(second_);
  }

  void subscribe(std::shared_ptr<Subscriber<T>> subscriber) override {
    auto subscription =
        std::make_shared<ConcatWithSubscription>(subscriber, first_, second_);
    subscription->init();
  }

 private:
  class ForwardSubscriber;

  // Downstream will always point to this subscription
  class ConcatWithSubscription
      : public yarpl::flowable::Subscription,
        public std::enable_shared_from_this<ConcatWithSubscription> {
   public:
    ConcatWithSubscription(
        std::shared_ptr<Subscriber<T>> subscriber,
        std::shared_ptr<Flowable<T>> first,
        std::shared_ptr<Flowable<T>> second)
        : downSubscriber_(std::move(subscriber)),
          first_(std::move(first)),
          second_(std::move(second)) {}

    void init() {
      upSubscriber_ =
          std::make_shared<ForwardSubscriber>(this->shared_from_this());
      first_->subscribe(upSubscriber_);
      downSubscriber_->onSubscribe(this->shared_from_this());
    }

    void request(int64_t n) override {
      credits::add(&requested_, n);
      upSubscriber_->request(n);
    }

    void cancel() override {
      if (auto subscriber = std::move(upSubscriber_)) {
        subscriber->cancel();
      }
    }

    void onNext(T value) {
      credits::consume(&requested_, 1);
      downSubscriber_->onNext(std::move(value));
    }

    void onComplete() {
      if (auto first = std::move(first_)) {
        upSubscriber_ =
            std::make_shared<ForwardSubscriber>(this->shared_from_this());
        second_->subscribe(upSubscriber_);
        if (requested_ > 0) {
          upSubscriber_->request(requested_);
        }
      } else {
        downSubscriber_->onComplete();
      }
    }

    void onError(folly::exception_wrapper ew) {
      downSubscriber_->onError(std::move(ew));
    }

   private:
    std::shared_ptr<Subscriber<T>> downSubscriber_;
    std::shared_ptr<Flowable<T>> first_;
    std::shared_ptr<Flowable<T>> second_;
    std::shared_ptr<ForwardSubscriber> upSubscriber_;
    std::atomic<int64_t> requested_{0};
  };

  class ForwardSubscriber : public yarpl::flowable::Subscriber<T>,
                            public yarpl::flowable::Subscription {
   public:
    ForwardSubscriber(
        std::shared_ptr<ConcatWithSubscription> concatWithSubscription)
        : concatWithSubscription_(std::move(concatWithSubscription)) {}

    void request(int64_t n) override {
      subscription_->request(n);
    }

    void cancel() override {
      if (auto subs = std::move(subscription_)) {
        subs->cancel();
      }
    }

    void onSubscribe(std::shared_ptr<Subscription> subscription) override {
      // Don't forward the subscription to downstream subscriber
      subscription_ = std::move(subscription);
    }

    void onComplete() override {
      concatWithSubscription_->onComplete();
    }

    void onError(folly::exception_wrapper ew) override {
      concatWithSubscription_->onError(std::move(ew));
    }
    void onNext(T value) override {
      concatWithSubscription_->onNext(std::move(value));
    }

   private:
    std::shared_ptr<ConcatWithSubscription> concatWithSubscription_;
    std::shared_ptr<flowable::Subscription> subscription_;
  };

 private:
  const std::shared_ptr<Flowable<T>> first_;
  const std::shared_ptr<Flowable<T>> second_;
};

} // namespace details
} // namespace flowable
} // namespace yarpl
