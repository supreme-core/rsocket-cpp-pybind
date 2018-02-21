// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gmock/gmock.h>

#include "rsocket/statemachine/StreamsWriter.h"

namespace rsocket {

class MockStreamsWriter : public StreamsWriter {
 public:
  MOCK_METHOD4(writeNewStream, void(StreamId, StreamType, uint32_t, Payload));
  MOCK_METHOD1(writeRequestN, void(rsocket::Frame_REQUEST_N&&));
  MOCK_METHOD1(writeCancel, void(rsocket::Frame_CANCEL&&));
  MOCK_METHOD1(writePayload, void(rsocket::Frame_PAYLOAD&&));
  MOCK_METHOD1(writeError, void(rsocket::Frame_ERROR&&));
  MOCK_METHOD1(onStreamClosed, void(rsocket::StreamId));
};
} // namespace rsocket
