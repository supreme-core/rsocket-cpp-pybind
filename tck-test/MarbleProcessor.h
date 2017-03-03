// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"

namespace reactivesocket {
namespace tck {

class MarbleProcessor {
 public:
  explicit MarbleProcessor(
      const std::string /* marble */,
      const std::shared_ptr<Subscriber<Payload>>&);

  void run();

  void request(size_t);

  void cancel();

 private:
  std::string marble_;

  // TODO(manikandan): RequestHandler callbacks take Subscriber wrapped inside
  // a const reference, which prevents copying of Subscriber.  Store raw pointer
  // now to get things working now.
  Subscriber<Payload>* subscriber_;

  // Keeps an account of how many messages can be sent.  This could be done
  // with Semaphores (AllowanceSemaphore)
  std::atomic<size_t> canSend_{0};

  // Indicates whether we can send onError/onComplete
  std::atomic<bool> canTerminate_{false};

  // Indicates whether the connection has been closed from the other end
  std::atomic<bool> terminated_{false};
};

} // tck
} // reactivesocket
