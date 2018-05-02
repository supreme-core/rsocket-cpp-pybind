// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include <iosfwd>

namespace rsocket {

// Bug in GCC: https://bugzilla.redhat.com/show_bug.cgi?id=130601
#pragma push_macro("major")
#pragma push_macro("minor")
#undef major
#undef minor

struct ProtocolVersion {
  uint16_t major{};
  uint16_t minor{};

  constexpr ProtocolVersion() = default;
  constexpr ProtocolVersion(uint16_t _major, uint16_t _minor)
      : major(_major), minor(_minor) {}

  static const ProtocolVersion Unknown;
  static const ProtocolVersion Latest;
};

#pragma pop_macro("major")
#pragma pop_macro("minor")

std::ostream& operator<<(std::ostream&, const ProtocolVersion&);

constexpr bool operator==(
    const ProtocolVersion& left,
    const ProtocolVersion& right) {
  return left.major == right.major && left.minor == right.minor;
}

constexpr bool operator!=(
    const ProtocolVersion& left,
    const ProtocolVersion& right) {
  return !(left == right);
}

constexpr bool operator<(
    const ProtocolVersion& left,
    const ProtocolVersion& right) {
  return left != ProtocolVersion::Unknown &&
      right != ProtocolVersion::Unknown &&
      (left.major < right.major ||
       (left.major == right.major && left.minor < right.minor));
}

constexpr bool operator>(
    const ProtocolVersion& left,
    const ProtocolVersion& right) {
  return left != ProtocolVersion::Unknown &&
      right != ProtocolVersion::Unknown &&
      (left.major > right.major ||
       (left.major == right.major && left.minor > right.minor));
}

} // namespace rsocket
