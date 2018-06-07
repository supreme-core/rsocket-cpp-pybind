// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/statemachine/StreamFragmentAccumulator.h"

namespace rsocket {

StreamFragmentAccumulator::StreamFragmentAccumulator()
    : flagsComplete(false), flagsNext(false) {}

void StreamFragmentAccumulator::addPayloadIgnoreFlags(Payload p) {
  if (p.metadata) {
    if (!fragments.metadata) {
      fragments.metadata = std::move(p.metadata);
    } else {
      fragments.metadata->prev()->appendChain(std::move(p.metadata));
    }
  }

  if (p.data) {
    if (!fragments.data) {
      fragments.data = std::move(p.data);
    } else {
      fragments.data->prev()->appendChain(std::move(p.data));
    }
  }
}

void StreamFragmentAccumulator::addPayload(
    Payload p,
    bool next,
    bool complete) {
  flagsNext |= next;
  flagsComplete |= complete;
  addPayloadIgnoreFlags(std::move(p));
}

Payload StreamFragmentAccumulator::consumePayloadIgnoreFlags() {
  flagsComplete = false;
  flagsNext = false;
  return std::move(fragments);
}

std::tuple<Payload, bool, bool>
StreamFragmentAccumulator::consumePayloadAndFlags() {
  auto ret = std::make_tuple(
      std::move(fragments), bool(flagsNext), bool(flagsComplete));
  flagsComplete = false;
  flagsNext = false;
  return ret;
}

} /* namespace rsocket */
