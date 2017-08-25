// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <utility>

#include "yarpl/Observable.h"
#include "yarpl/observable/Observer.h"
#include "yarpl/observable/Subscription.h"

namespace yarpl {
namespace observable {

/**
 * Base (helper) class for operators.  Operators are templated on two types:
 * D (downstream) and U (upstream).  Operators are created by method calls on
 * an upstream Observable, and are Observables themselves.  Multi-stage
 * pipelines
 * can be built: a Observable heading a sequence of Operators.
 */
template <typename U, typename D, typename ThisOperatorT>
class ObservableOperator : public Observable<D> {
 public:
  explicit ObservableOperator(Reference<Observable<U>> upstream)
      : upstream_(std::move(upstream)) {}

 protected:
  /// An Operator's subscription.
  ///
  /// When a pipeline chain is active, each Observable has a corresponding
  /// subscription.  Except for the first one, the subscriptions are created
  /// against Operators.  Each operator subscription has two functions: as a
  /// subscriber for the previous stage; as a subscription for the next one,
  /// the user-supplied subscriber being the last of the pipeline stages.
  class Subscription : public ::yarpl::observable::Subscription,
                       public Observer<U> {
   protected:
    Subscription(
        Reference<ThisOperatorT> observable,
        Reference<Observer<D>> observer)
        : observable_(std::move(observable)), observer_(std::move(observer)) {
      assert(observable_);
      assert(observer_);
    }

    Reference<ThisOperatorT>& getObservableOperator() {
      static_assert(
          std::is_base_of<Observable<D>, ThisOperatorT>::value,
          "Operator must be a subclass of Observable<D>");
      return observable_;
    }

    void observerOnNext(D value) {
      if (observer_) {
        observer_->onNext(std::move(value));
      }
    }

    /// Terminates both ends of an operator normally.
    void terminate() {
      terminateImpl(TerminateState::Both());
    }

    /// Terminates both ends of an operator with an error.
    void terminateErr(folly::exception_wrapper ex) {
      terminateImpl(TerminateState::Both(), std::move(ex));
    }

    // Subscription.

    void cancel() override {
      terminateImpl(TerminateState::Up());
    }

    // Observer.

    void onSubscribe(
        Reference<yarpl::observable::Subscription> subscription) override {
      if (upstream_) {
        subscription->cancel();
        return;
      }

      upstream_ = std::move(subscription);
      observer_->onSubscribe(get_ref(this));
    }

    void onComplete() override {
      terminateImpl(TerminateState::Down());
    }

    void onError(folly::exception_wrapper ex) override {
      terminateImpl(TerminateState::Down(), std::move(ex));
    }

   private:
    struct TerminateState {
      TerminateState(bool u, bool d) : up{u}, down{d} {}

      static TerminateState Down() {
        return TerminateState{false, true};
      }

      static TerminateState Up() {
        return TerminateState{true, false};
      }

      static TerminateState Both() {
        return TerminateState{true, true};
      }

      const bool up{false};
      const bool down{false};
    };

    bool isTerminated() const {
      return !upstream_ && !observer_;
    }

    /// Terminates an operator, sending cancel() and on{Complete,Error}()
    /// signals as necessary.
    void terminateImpl(
        TerminateState state,
        folly::exception_wrapper ex = folly::exception_wrapper{nullptr}) {
      if (isTerminated()) {
        return;
      }

      if (auto upstream = std::move(upstream_)) {
        if (state.up) {
          upstream->cancel();
        }
      }

      if (auto observer = std::move(observer_)) {
        if (state.down) {
          if (ex) {
            observer->onError(std::move(ex));
          } else {
            observer->onComplete();
          }
        }
      }
    }

    /// The Observable has the lambda, and other creation parameters.
    Reference<ThisOperatorT> observable_;

    /// This subscription controls the life-cycle of the observer. The
    /// observer is retained as long as calls on it can be made.  (Note:
    /// the observer in turn maintains a reference on this subscription
    /// object until cancellation and/or completion.)
    Reference<Observer<D>> observer_;

    /// In an active pipeline, cancel and (possibly modified) request(n)
    /// calls should be forwarded upstream.  Note that `this` is also a
    /// observer for the upstream stage: thus, there are cycles; all of
    /// the objects drop their references at cancel/complete.
    Reference<::yarpl::observable::Subscription> upstream_;
  };

  Reference<Observable<U>> upstream_;
};

template <
    typename U,
    typename D,
    typename F,
    typename = typename std::enable_if<std::is_callable<F(U), D>::value>::type>
class MapOperator : public ObservableOperator<U, D, MapOperator<U, D, F>> {
  using ThisOperatorT = MapOperator<U, D, F>;
  using Super = ObservableOperator<U, D, ThisOperatorT>;

 public:
  MapOperator(Reference<Observable<U>> upstream, F function)
      : Super(std::move(upstream)), function_(std::move(function)) {}

  void subscribe(Reference<Observer<D>> observer) override {
    Super::upstream_->subscribe(
        // Note: implicit cast to a reference to a observer.
        make_ref<Subscription>(get_ref(this), std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        Reference<Observer<D>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)) {}

    void onNext(U value) override {
      auto& map = Super::Subscription::getObservableOperator();
      Super::Subscription::observerOnNext(map->function_(std::move(value)));
    }
  };

  F function_;
};

template <
    typename U,
    typename F,
    typename =
        typename std::enable_if<std::is_callable<F(U), bool>::value>::type>
class FilterOperator : public ObservableOperator<U, U, FilterOperator<U, F>> {
  using ThisOperatorT = FilterOperator<U, F>;
  using Super = ObservableOperator<U, U, ThisOperatorT>;

 public:
  FilterOperator(Reference<Observable<U>> upstream, F function)
      : Super(std::move(upstream)), function_(std::move(function)) {}

  void subscribe(Reference<Observer<U>> observer) override {
    Super::upstream_->subscribe(
        // Note: implicit cast to a reference to a observer.
        make_ref<Subscription>(get_ref(this), std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        Reference<Observer<U>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)) {}

    void onNext(U value) override {
      auto& filter = Super::Subscription::getObservableOperator();
      if (filter->function_(value)) {
        Super::Subscription::observerOnNext(std::move(value));
      }
    }
  };

  F function_;
};

template <
    typename U,
    typename D,
    typename F,
    typename = typename std::enable_if<std::is_assignable<D, U>::value>,
    typename =
        typename std::enable_if<std::is_callable<F(D, U), D>::value>::type>
class ReduceOperator
    : public ObservableOperator<U, D, ReduceOperator<U, D, F>> {
  using ThisOperatorT = ReduceOperator<U, D, F>;
  using Super = ObservableOperator<U, D, ThisOperatorT>;

 public:
  ReduceOperator(Reference<Observable<U>> upstream, F function)
      : Super(std::move(upstream)), function_(std::move(function)) {}

  void subscribe(Reference<Observer<D>> subscriber) override {
    Super::upstream_->subscribe(
        // Note: implicit cast to a reference to a subscriber.
        make_ref<Subscription>(get_ref(this), std::move(subscriber)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> flowable,
        Reference<Observer<D>> subscriber)
        : Super::Subscription(std::move(flowable), std::move(subscriber)),
          accInitialized_(false) {}

    void onNext(U value) override {
      auto& reduce = Super::Subscription::getObservableOperator();
      if (accInitialized_) {
        acc_ = reduce->function_(std::move(acc_), std::move(value));
      } else {
        acc_ = std::move(value);
        accInitialized_ = true;
      }
    }

    void onComplete() override {
      if (accInitialized_) {
        Super::Subscription::observerOnNext(std::move(acc_));
      }
      Super::Subscription::onComplete();
    }

   private:
    bool accInitialized_;
    D acc_;
  };

  F function_;
};

template <typename T>
class TakeOperator : public ObservableOperator<T, T, TakeOperator<T>> {
  using ThisOperatorT = TakeOperator<T>;
  using Super = ObservableOperator<T, T, ThisOperatorT>;

 public:
  TakeOperator(Reference<Observable<T>> upstream, int64_t limit)
      : Super(std::move(upstream)), limit_(limit) {}

  void subscribe(Reference<Observer<T>> observer) override {
    Super::upstream_->subscribe(
        make_ref<Subscription>(get_ref(this), limit_, std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        int64_t limit,
        Reference<Observer<T>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)),
          limit_(limit) {}

    void onNext(T value) override {
      if (limit_-- > 0) {
        if (pending_ > 0)
          --pending_;
        Super::Subscription::observerOnNext(std::move(value));
        if (limit_ == 0) {
          Super::Subscription::terminate();
        }
      }
    }

   private:
    int64_t pending_{0};
    int64_t limit_;
  };

  const int64_t limit_;
};

template <typename T>
class SkipOperator : public ObservableOperator<T, T, SkipOperator<T>> {
  using ThisOperatorT = SkipOperator<T>;
  using Super = ObservableOperator<T, T, ThisOperatorT>;

 public:
  SkipOperator(Reference<Observable<T>> upstream, int64_t offset)
      : Super(std::move(upstream)), offset_(offset) {}

  void subscribe(Reference<Observer<T>> observer) override {
    Super::upstream_->subscribe(
        make_ref<Subscription>(get_ref(this), offset_, std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        int64_t offset,
        Reference<Observer<T>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)),
          offset_(offset) {}

    void onNext(T value) override {
      if (offset_ <= 0) {
        Super::Subscription::observerOnNext(std::move(value));
      } else {
        --offset_;
      }
    }

   private:
    int64_t offset_;
  };

  const int64_t offset_;
};

template <typename T>
class IgnoreElementsOperator
    : public ObservableOperator<T, T, IgnoreElementsOperator<T>> {
  using ThisOperatorT = IgnoreElementsOperator<T>;
  using Super = ObservableOperator<T, T, ThisOperatorT>;

 public:
  explicit IgnoreElementsOperator(Reference<Observable<T>> upstream)
      : Super(std::move(upstream)) {}

  void subscribe(Reference<Observer<T>> observer) override {
    Super::upstream_->subscribe(
        make_ref<Subscription>(get_ref(this), std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        Reference<Observer<T>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)) {}

    void onNext(T) override {}
  };
};

template <typename T>
class SubscribeOnOperator
    : public ObservableOperator<T, T, SubscribeOnOperator<T>> {
  using ThisOperatorT = SubscribeOnOperator<T>;
  using Super = ObservableOperator<T, T, ThisOperatorT>;

 public:
  SubscribeOnOperator(Reference<Observable<T>> upstream, Scheduler& scheduler)
      : Super(std::move(upstream)), worker_(scheduler.createWorker()) {}

  void subscribe(Reference<Observer<T>> observer) override {
    Super::upstream_->subscribe(make_ref<Subscription>(
        get_ref(this), std::move(worker_), std::move(observer)));
  }

 private:
  class Subscription : public Super::Subscription {
   public:
    Subscription(
        Reference<ThisOperatorT> observable,
        std::unique_ptr<Worker> worker,
        Reference<Observer<T>> observer)
        : Super::Subscription(std::move(observable), std::move(observer)),
          worker_(std::move(worker)) {}

    void cancel() override {
      worker_->schedule([this] { this->callSuperCancel(); });
    }

    void onNext(T value) override {
      auto observer = Super::Subscription::observer_;
      observer->onNext(std::move(value));
    }

   private:
    // Trampoline to call superclass method; gcc bug 58972.
    void callSuperCancel() {
      Super::Subscription::cancel();
    }

    std::unique_ptr<Worker> worker_;
  };

  std::unique_ptr<Worker> worker_;
};

template <typename T, typename OnSubscribe>
class FromPublisherOperator : public Observable<T> {
 public:
  explicit FromPublisherOperator(OnSubscribe function)
      : function_(std::move(function)) {}

  void subscribe(Reference<Observer<T>> observer) override {
    function_(std::move(observer));
  }

 private:
  OnSubscribe function_;
};

} // observable
} // yarpl
