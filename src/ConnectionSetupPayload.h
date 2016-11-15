// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/io/IOBuf.h>
#include <string>
#include "Payload.h"
#include "StreamState.h"

namespace reactivesocket {
class ConnectionSetupPayload {
 public:
  ConnectionSetupPayload(
      std::string _metadataMimeType = "",
      std::string _dataMimeType = "",
      Payload _payload = Payload(),
      const ResumeIdentificationToken& token =
          { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } })
      : metadataMimeType(std::move(_metadataMimeType)),
        dataMimeType(std::move(_dataMimeType)),
        payload(std::move(_payload)),
        token(token) {};

  std::string metadataMimeType;
  std::string dataMimeType;
  Payload payload;
  const ResumeIdentificationToken token;
};

std::ostream& operator<<(std::ostream&, const ConnectionSetupPayload&);
}
