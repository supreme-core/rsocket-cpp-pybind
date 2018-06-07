// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "rsocket/Payload.h"

namespace rsocket {

class StreamFragmentAccumulator {
 public:
  StreamFragmentAccumulator();

  void addPayloadIgnoreFlags(Payload p);
  void addPayload(Payload p, bool next, bool complete);

  Payload consumePayloadIgnoreFlags();
  std::tuple<Payload, bool, bool> consumePayloadAndFlags();

  bool anyFragments() const {
    return fragments.data || fragments.metadata;
  }

 private:
  bool flagsComplete : 1;
  bool flagsNext : 1;
  Payload fragments;
};

} /* namespace rsocket */
