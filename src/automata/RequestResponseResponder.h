// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <iosfwd>

#include "reactive-streams/utilities/SmartPointers.h"
#include "src/Frame.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/automata/StreamSubscriptionResponderBase.h"
#include "src/mixins/ExecutorMixin.h"
#include "src/mixins/LoggingMixin.h"
#include "src/mixins/MemoryMixin.h"
#include "src/mixins/SinkIfMixin.h"
#include "src/mixins/StreamIfMixin.h"

namespace folly {
class exception_wrapper;
}

namespace reactivesocket {

enum class StreamCompletionSignal;

/// Implementation of stream automaton that represents a RequestResponse
/// responder
class RequestResponseResponderBase : public StreamSubscriptionResponderBase {
  using Base = StreamSubscriptionResponderBase;

 public:
  using Base::Base;

 protected:
  /// @{

  /// Don't need to intercept any frames, let them pass through.
  using Base::onNextFrame;

  std::ostream& logPrefix(std::ostream& os) {
    return os << "RequestResponseResponder(" << &connection_ << ", "
              << streamId_ << "): ";
  }
  /// @}
};

using RequestResponseResponder =
    SinkIfMixin<StreamIfMixin<LoggingMixin<ExecutorMixin<LoggingMixin<
        MemoryMixin<LoggingMixin<RequestResponseResponderBase>>>>>>>;
}
