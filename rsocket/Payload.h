// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/IOBuf.h>
#include <memory>
#include <string>

namespace rsocket {

/// The type of a read-only view on a binary buffer.
/// MUST manage the lifetime of the underlying buffer.
struct Payload {
  Payload() = default;

  explicit Payload(
      std::unique_ptr<folly::IOBuf> data,
      std::unique_ptr<folly::IOBuf> metadata = std::unique_ptr<folly::IOBuf>());

  explicit Payload(
      folly::StringPiece data,
      folly::StringPiece metadata = folly::StringPiece{});

  explicit operator bool() const {
    return data != nullptr || metadata != nullptr;
  }

  std::string moveDataToString();
  std::string cloneDataToString() const;

  std::string moveMetadataToString();
  std::string cloneMetadataToString() const;

  void clear();

  Payload clone() const;

  std::unique_ptr<folly::IOBuf> data;
  std::unique_ptr<folly::IOBuf> metadata;
};

std::ostream& operator<<(std::ostream& os, const Payload& payload);

} // namespace rsocket
