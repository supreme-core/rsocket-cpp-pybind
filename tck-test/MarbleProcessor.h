// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <map>

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

  // Stores a mapping from marble character to Payload (data, metadata)
  std::map<std::string, std::pair<std::string, std::string>> argMap_;

  // RequestHandler callbacks take Subscriber wrapped inside
  // a const reference, which prevents copying of Subscriber.
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
