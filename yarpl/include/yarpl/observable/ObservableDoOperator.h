// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace yarpl {
namespace observable {

template <
    typename U,
    typename OnSubscribeFunc,
    typename OnNextFunc,
    typename OnErrorFunc,
    typename OnCompleteFunc>
class DoOperator : public ObservableOperator<U, U> {
  using Super = ObservableOperator<U, U>;

 public:
  DoOperator(std::shared_ptr<Observable<U>> upstream,
             OnSubscribeFunc onSubscribeFunc,
             OnNextFunc onNextFunc,
             OnErrorFunc onErrorFunc,
             OnCompleteFunc onCompleteFunc)
      : Super(std::move(upstream)),
        onSubscribeFunc_(std::move(onSubscribeFunc)),
        onNextFunc_(std::move(onNextFunc)),
        onErrorFunc_(std::move(onErrorFunc)),
        onCompleteFunc_(std::move(onCompleteFunc)) {}

  std::shared_ptr<Subscription> subscribe(
      std::shared_ptr<Observer<U>> observer) override {
    auto subscription = std::make_shared<DoSubscription>(
        this->ref_from_this(this), std::move(observer));
    Super::upstream_->subscribe(
        // Note: implicit cast to a reference to a observer.
        subscription);
    return subscription;
  }

 private:
  class DoSubscription : public Super::OperatorSubscription {
    using SuperSub = typename Super::OperatorSubscription;

   public:
    DoSubscription(
        std::shared_ptr<DoOperator> observable,
        std::shared_ptr<Observer<U>> observer)
        : SuperSub(std::move(observer)), observable_(std::move(observable)) {}

    void onSubscribe(std::shared_ptr<yarpl::observable::Subscription>
                         subscription) override {
      observable_->onSubscribeFunc_();
      SuperSub::onSubscribe(std::move(subscription));
    }

    void onNext(U value) override {
      const auto& valueRef = value;
      observable_->onNextFunc_(valueRef);
      SuperSub::observerOnNext(std::move(value));
    }

    void onError(folly::exception_wrapper ex) override {
      const auto& exRef = ex;
      observable_->onErrorFunc_(exRef);
      SuperSub::onError(std::move(ex));
    }

    void onComplete() override {
      observable_->onCompleteFunc_();
      SuperSub::onComplete();
    }

  private:
    std::shared_ptr<DoOperator> observable_;
  };

  OnSubscribeFunc onSubscribeFunc_;
  OnNextFunc onNextFunc_;
  OnErrorFunc onErrorFunc_;
  OnCompleteFunc onCompleteFunc_;
};

namespace details {

template <
    typename U,
    typename OnSubscribeFunc,
    typename OnNextFunc,
    typename OnErrorFunc,
    typename OnCompleteFunc>
inline auto createDoOperator(std::shared_ptr<Observable<U>> upstream,
                             OnSubscribeFunc onSubscribeFunc,
                             OnNextFunc onNextFunc,
                             OnErrorFunc onErrorFunc,
                             OnCompleteFunc onCompleteFunc) {
  return std::make_shared<
      DoOperator<U, OnSubscribeFunc, OnNextFunc, OnErrorFunc, OnCompleteFunc>>(
      std::move(upstream),
      std::move(onSubscribeFunc),
      std::move(onNextFunc),
      std::move(onErrorFunc),
      std::move(onCompleteFunc));
}
}
}
}
