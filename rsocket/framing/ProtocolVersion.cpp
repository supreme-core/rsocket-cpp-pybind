// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/framing/ProtocolVersion.h"

#include <limits>
#include <ostream>

namespace rsocket {

const ProtocolVersion ProtocolVersion::Unknown = ProtocolVersion(
    std::numeric_limits<uint16_t>::max(),
    std::numeric_limits<uint16_t>::max());

const ProtocolVersion ProtocolVersion::Latest = ProtocolVersion(1, 0);

std::ostream& operator<<(std::ostream& os, const ProtocolVersion& version) {
  return os << version.major << "." << version.minor;
}

} // namespace rsocket
