// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/Stats.h"

namespace reactivesocket {
Stats& Stats::noop() {
  static NoopStats noop_;
  return noop_;
};
}
