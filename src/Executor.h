// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Executor.h>
#include <memory>

namespace reactivesocket {

folly::Executor& defaultExecutor();

class ExecutorBase {
 public:
  explicit ExecutorBase(folly::Executor& executor);

 protected:
  void runInExecutor(folly::Func func);

  folly::Executor& executor() {
    return executor_;
  }

 private:
  folly::Executor& executor_;
};

} // reactivesocket
