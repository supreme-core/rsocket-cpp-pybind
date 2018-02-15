// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

namespace yarpl {
namespace flowable {
namespace details {

template <
    typename U,
    typename OnSubscribeFunc,
    typename OnNextFunc,
    typename OnErrorFunc,
    typename OnCompleteFunc,
    typename OnRequestFunc,
    typename OnCancelFunc>
class DoOperator : public FlowableOperator<U, U> {
  using Super = FlowableOperator<U, U>;

 public:
  DoOperator(
      std::shared_ptr<Flowable<U>> upstream,
      OnSubscribeFunc onSubscribeFunc,
      OnNextFunc onNextFunc,
      OnErrorFunc onErrorFunc,
      OnCompleteFunc onCompleteFunc,
      OnRequestFunc onRequestFunc,
      OnCancelFunc onCancelFunc)
      : upstream_(std::move(upstream)),
        onSubscribeFunc_(std::move(onSubscribeFunc)),
        onNextFunc_(std::move(onNextFunc)),
        onErrorFunc_(std::move(onErrorFunc)),
        onCompleteFunc_(std::move(onCompleteFunc)),
        onRequestFunc_(std::move(onRequestFunc)),
        onCancelFunc_(std::move(onCancelFunc)) {}

  void subscribe(std::shared_ptr<Subscriber<U>> subscriber) override {
    auto subscription = std::make_shared<DoSubscription>(
        this->ref_from_this(this), std::move(subscriber));
    upstream_->subscribe(
        // Note: implicit cast to a reference to a subscriber.
        subscription);
  }

 private:
  class DoSubscription : public Super::Subscription {
    using SuperSub = typename Super::Subscription;

   public:
    DoSubscription(
        std::shared_ptr<DoOperator> flowable,
        std::shared_ptr<Subscriber<U>> subscriber)
        : SuperSub(std::move(subscriber)), flowable_(std::move(flowable)) {}

    void onSubscribeImpl() override {
      flowable_->onSubscribeFunc_();
      SuperSub::onSubscribeImpl();
    }

    void onNextImpl(U value) override {
      const auto& valueRef = value;
      flowable_->onNextFunc_(valueRef);
      SuperSub::subscriberOnNext(std::move(value));
    }

    void onErrorImpl(folly::exception_wrapper ex) override {
      const auto& exRef = ex;
      flowable_->onErrorFunc_(exRef);
      SuperSub::onErrorImpl(std::move(ex));
    }

    void onCompleteImpl() override {
      flowable_->onCompleteFunc_();
      SuperSub::onCompleteImpl();
    }

    void cancel() override {
      flowable_->onCancelFunc_();
      SuperSub::cancel();
    }

    void request(int64_t n) override {
      flowable_->onRequestFunc_(n);
      SuperSub::request(n);
    }

   private:
    std::shared_ptr<DoOperator> flowable_;
  };

  std::shared_ptr<Flowable<U>> upstream_;
  OnSubscribeFunc onSubscribeFunc_;
  OnNextFunc onNextFunc_;
  OnErrorFunc onErrorFunc_;
  OnCompleteFunc onCompleteFunc_;
  OnRequestFunc onRequestFunc_;
  OnCancelFunc onCancelFunc_;
};

template <
    typename U,
    typename OnSubscribeFunc,
    typename OnNextFunc,
    typename OnErrorFunc,
    typename OnCompleteFunc,
    typename OnRequestFunc,
    typename OnCancelFunc>
inline auto createDoOperator(
    std::shared_ptr<Flowable<U>> upstream,
    OnSubscribeFunc onSubscribeFunc,
    OnNextFunc onNextFunc,
    OnErrorFunc onErrorFunc,
    OnCompleteFunc onCompleteFunc,
    OnRequestFunc onRequestFunc,
    OnCancelFunc onCancelFunc) {
  return std::make_shared<DoOperator<
      U,
      OnSubscribeFunc,
      OnNextFunc,
      OnErrorFunc,
      OnCompleteFunc,
      OnRequestFunc,
      OnCancelFunc>>(
      std::move(upstream),
      std::move(onSubscribeFunc),
      std::move(onNextFunc),
      std::move(onErrorFunc),
      std::move(onCompleteFunc),
      std::move(onRequestFunc),
      std::move(onCancelFunc));
}
} // namespace details
} // namespace flowable
} // namespace yarpl
