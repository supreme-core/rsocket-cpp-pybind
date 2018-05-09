// Copyright 2004-present Facebook. All Rights Reserved.

#include "yarpl/observable/Observables.h"

namespace yarpl {
namespace observable {

std::shared_ptr<Observable<int64_t>> Observable<>::range(
    int64_t start,
    int64_t count) {
  auto lambda = [start, count](std::shared_ptr<Observer<int64_t>> observer) {
    auto end = start + count;
    for (int64_t i = start; i < end; ++i) {
      observer->onNext(i);
    }
    observer->onComplete();
  };

  return Observable<int64_t>::create(std::move(lambda));
}
} // namespace observable
} // namespace yarpl
