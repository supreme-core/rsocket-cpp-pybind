#include "rsocket/framing/ScheduledFrameProcessor.h"

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>

namespace rsocket {

ScheduledFrameProcessor::~ScheduledFrameProcessor() {}

void ScheduledFrameProcessor::processFrame(
    std::unique_ptr<folly::IOBuf> ioBuf) {
  evb_->runInEventBaseThread(
      [fp = frameProcessor_, ioBuf = std::move(ioBuf)]() mutable {
        fp->processFrame(std::move(ioBuf));
      });
}

void ScheduledFrameProcessor::onTerminal(folly::exception_wrapper ex) {
  evb_->runInEventBaseThread(
      [ex = std::move(ex), fp = frameProcessor_]() mutable {
        fp->onTerminal(std::move(ex));
      });
}
} // namespace rsocket
