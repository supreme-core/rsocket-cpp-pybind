// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Synchronized.h>
#include <folly/synchronization/Baton.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include "rsocket/statemachine/RSocketStateMachine.h"

namespace folly {
class EventBase;
}

namespace rsocket {

/// Set of RSocketStateMachine objects.  Stores them until they call
/// RSocketStateMachine::close().
///
/// Also tracks which EventBase is controlling each state machine so that they
/// can be closed on the correct thread.
class ConnectionSet : public RSocketStateMachine::CloseCallback {
 public:
  ConnectionSet();
  virtual ~ConnectionSet();

  bool insert(std::shared_ptr<RSocketStateMachine>, folly::EventBase*);
  void remove(RSocketStateMachine&) override;

  size_t size() const;

  void shutdownAndWait();

 private:
  using StateMachineMap = std::
      unordered_map<std::shared_ptr<RSocketStateMachine>, folly::EventBase*>;

  folly::Synchronized<StateMachineMap, std::mutex> machines_;
  folly::Baton<> shutdownDone_;
  size_t removes_{0};
  size_t targetRemoves_{0};
  std::atomic<bool> shutDown_{false};
};

} // namespace rsocket
