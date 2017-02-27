// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <iostream>
#include <memory>
#include <string>

class FlowableExamples {
 public:
  static void run();
};

/*********************** ASubscription **************/

class ASubscription {
 public:
  virtual ~ASubscription(){};
  virtual void cancel() = 0;
  virtual void request(uint64_t n) = 0;
};

/*********************** ASubscriber ****************/

template <typename T>
class ASubscriber {
 public:
  virtual ~ASubscriber(){};
  virtual void onNext(const T& value) = 0;
  virtual void onError(const std::exception& e) = 0;
  virtual void onComplete() = 0;
  virtual void onSubscribe(std::unique_ptr<ASubscription>) = 0;
};

/*********************** AFlowable ******************/

template <typename T>
class AFlowable {
  std::function<void(std::unique_ptr<ASubscriber<T>>)> onSubscribe;

 public:
  ~AFlowable();
  explicit AFlowable(
      std::function<void(std::unique_ptr<ASubscriber<T>>)> onSubscribe);
  static std::unique_ptr<AFlowable<T>> create(
      std::function<void(std::unique_ptr<ASubscriber<T>>)> onSubscribe);
  void subscribe(std::unique_ptr<ASubscriber<T>>);
};

template <typename T>
AFlowable<T>::~AFlowable() {
  std::cout << "AFlowable DESTROYED" << std::endl;
}

template <typename T>
AFlowable<T>::AFlowable(std::function<void(std::unique_ptr<ASubscriber<T>>)> os)
    : onSubscribe(os) {
  std::cout << "AFlowable CREATED" << std::endl;
};

template <typename T>
std::unique_ptr<AFlowable<T>> AFlowable<T>::create(
    std::function<void(std::unique_ptr<ASubscriber<T>>)> onSubscribe) {
  return std::make_unique<AFlowable>(onSubscribe);
}

template <typename T>
void AFlowable<T>::subscribe(std::unique_ptr<ASubscriber<T>> o) {
  // when subscribed to, invoke the `onSubscribe` function
  onSubscribe(std::move(o));
}
