// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/IOBuf.h>
#include <string>
#include "src/Payload.h"
#include "src/StreamState.h"

namespace reactivesocket {
class ConnectionSetupPayload {
 public:
  explicit ConnectionSetupPayload(
      std::string _metadataMimeType = "",
      std::string _dataMimeType = "",
      Payload _payload = Payload(),
      const ResumeIdentificationToken& _token =
          ResumeIdentificationToken::generateNew())
      : metadataMimeType(std::move(_metadataMimeType)),
        dataMimeType(std::move(_dataMimeType)),
        payload(std::move(_payload)),
        token(_token){};

  std::string metadataMimeType;
  std::string dataMimeType;
  Payload payload;
  // TODO(lehecka) the value is already specified in the ReactiveSocket ctor
  // we should set it only once
  const ResumeIdentificationToken token;
};

std::ostream& operator<<(std::ostream&, const ConnectionSetupPayload&);
}
