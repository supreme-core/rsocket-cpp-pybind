// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gmock/gmock.h>

#include "rsocket/RSocketStats.h"
#include "rsocket/framing/FrameSerializer_v1_0.h"
#include "rsocket/statemachine/StreamsWriter.h"

namespace rsocket {

class MockStreamsWriterImpl : public StreamsWriterImpl {
 public:
  MOCK_METHOD1(onStreamClosed, void(StreamId));
  MOCK_METHOD1(outputFrame, void(std::unique_ptr<folly::IOBuf>));
  MOCK_METHOD0(shouldQueue, bool());

  MockStreamsWriterImpl() {
    using namespace testing;
    ON_CALL(*this, shouldQueue()).WillByDefault(Invoke([this]() {
      return this->shouldQueue_;
    }));
  }

  FrameSerializer& serializer() override {
    return frameSerializer;
  }

  RSocketStats& stats() override {
    return *stats_;
  }

  using StreamsWriterImpl::sendPendingFrames;

  bool shouldQueue_{false};
  std::shared_ptr<RSocketStats> stats_ = RSocketStats::noop();
  FrameSerializerV1_0 frameSerializer;
};

class MockStreamsWriter : public StreamsWriter {
 public:
  MOCK_METHOD4(writeNewStream, void(StreamId, StreamType, uint32_t, Payload));
  MOCK_METHOD1(writeRequestN, void(rsocket::Frame_REQUEST_N&&));
  MOCK_METHOD1(writeCancel, void(rsocket::Frame_CANCEL&&));
  MOCK_METHOD1(writePayload, void(rsocket::Frame_PAYLOAD&&));
  MOCK_METHOD1(writeError, void(rsocket::Frame_ERROR&&));
  MOCK_METHOD1(onStreamClosed, void(rsocket::StreamId));

  // Delegate the Mock calls to the implementation in StreamsWriterImpl.
  MockStreamsWriterImpl& delegateToImpl() {
    using namespace testing;
    ON_CALL(*this, writeNewStream(_, _, _, _))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::writeNewStream));
    ON_CALL(*this, writeRequestN(_))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::writeRequestN));
    ON_CALL(*this, writeCancel(_))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::writeCancel));
    ON_CALL(*this, writePayload(_))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::writePayload));
    ON_CALL(*this, writeError(_))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::writeError));
    ON_CALL(*this, onStreamClosed(_))
        .WillByDefault(Invoke(&impl_, &StreamsWriter::onStreamClosed));
    return impl_;
  }

 protected:
  MockStreamsWriterImpl impl_;
};

} // namespace rsocket
