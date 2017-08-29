// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <deque>
#include <folly/Synchronized.h>
#include "yarpl/Flowable.h"
#include "yarpl/utils/credits.h"

namespace yarpl {
namespace observable {
template <typename T>
class Observable;
}
}

namespace yarpl {
namespace flowable {

// Exception thrown in case the downstream can't keep up.
class MissingBackpressureException : public std::runtime_error {
 public:
  MissingBackpressureException() : std::runtime_error("BACK_PRESSURE: DROP (missing credits onNext)") {}
};

namespace details {

template <typename T>
class FlowableFromObservableSubscription
    : public flowable::Subscription,
      public observable::Observer<T> {
 public:
  FlowableFromObservableSubscription(
      Reference<observable::Observable<T>> observable,
      Reference<flowable::Subscriber<T>> subscriber)
      : observable_(std::move(observable)), subscriber_(std::move(subscriber)) {}

  FlowableFromObservableSubscription(FlowableFromObservableSubscription&&) =
      delete;

  FlowableFromObservableSubscription(
      const FlowableFromObservableSubscription&) = delete;
  FlowableFromObservableSubscription& operator=(
      FlowableFromObservableSubscription&&) = delete;
  FlowableFromObservableSubscription& operator=(
      const FlowableFromObservableSubscription&) = delete;

  void request(int64_t n) override {
    if (n <= 0) {
      return;
    }
    auto const r = credits::add(&requested_, n);
    if (r <= 0) {
      return;
    }

    if (!started.exchange(true)) {
      observable_->subscribe(get_ref(this));
    }

    onCreditsAvailable(r);
  }

  void cancel() override {
    if (credits::cancel(&requested_)) {
      // if this is the first time calling cancel, send the cancel
      observableSubscription_->cancel();
    }
  }

  // Observer override
  void onSubscribe(
      Reference<observable::Subscription> subscription) override {
    observableSubscription_ = subscription;
  }

  // Observer override
  void onNext(T t) override {
    if (requested_ > 0) {
      subscriber_->onNext(std::move(t));
      credits::consume(&requested_, 1);
      return;
    }
    onNextWithoutCredits(std::move(t));
  }

  // Observer override
  void onComplete() override {
    subscriber_->onComplete();
  }

  // Observer override
  void onError(folly::exception_wrapper error) override {
    subscriber_->onError(std::move(error));
  }

 protected:
  virtual void onCreditsAvailable(int64_t /*credits*/) {}
  virtual void onNextWithoutCredits(T /*t*/) {
    // by default drop anything else received while we don't have credits
  }

  Reference<observable::Observable<T>> observable_;
  Reference<flowable::Subscriber<T>> subscriber_;
  Reference<observable::Subscription> observableSubscription_;
  std::atomic_bool started{false};
  std::atomic<int64_t> requested_{0};
};

template <typename T>
using FlowableFromObservableSubscriptionDropStrategy = FlowableFromObservableSubscription<T>;

template <typename T>
class FlowableFromObservableSubscriptionErrorStrategy : public FlowableFromObservableSubscription<T> {
 using Super = FlowableFromObservableSubscription<T>;
 public:
  using Super::FlowableFromObservableSubscription;

 private:
  void onNextWithoutCredits(T /*t*/) override {
    Super::cancel();
    Super::onError(MissingBackpressureException());
  }
};

template <typename T>
class FlowableFromObservableSubscriptionBufferStrategy : public FlowableFromObservableSubscription<T> {
  using Super = FlowableFromObservableSubscription<T>;
 public:
  using Super::FlowableFromObservableSubscription;

 private:
  void onNextWithoutCredits(T t) override {
    buffer_->push_back(std::move(t));
  }

  void onCreditsAvailable(int64_t credits) override {
    DCHECK(credits > 0);
    auto&& lockedBuffer = buffer_.wlock();
    while(credits-- > 0 && !lockedBuffer->empty()) {
      Super::onNext(lockedBuffer->front());
      lockedBuffer->pop_front();
    }
  }

  folly::Synchronized<std::deque<T>> buffer_;
};

template <typename T>
class FlowableFromObservableSubscriptionLatestStrategy : public FlowableFromObservableSubscription<T> {
  using Super = FlowableFromObservableSubscription<T>;
 public:
  using Super::FlowableFromObservableSubscription;

 private:
  void onNextWithoutCredits(T t) override {
    storesLatest_ = true;
    *latest_.wlock() = std::move(t);
  }

  void onCreditsAvailable(int64_t credits) override {
    DCHECK(credits > 0);
    if(storesLatest_) {
      storesLatest_ = false;
      Super::onNext(std::move(*latest_.wlock()));
    }
  }

  std::atomic<bool> storesLatest_{false};
  folly::Synchronized<T> latest_;
};

template <typename T>
class FlowableFromObservableSubscriptionMissingStrategy : public FlowableFromObservableSubscription<T> {
  using Super = FlowableFromObservableSubscription<T>;
 public:
  using Super::FlowableFromObservableSubscription;

 private:
  void onNextWithoutCredits(T t) override {
    Super::onNext(std::move(t));
  }
};

}
}
}
