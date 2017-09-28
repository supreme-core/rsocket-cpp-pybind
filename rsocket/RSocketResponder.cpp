// Copyright 2004-present Facebook. All Rights Reserved.

#include "rsocket/RSocketResponder.h"

#include <folly/io/async/EventBase.h>

namespace rsocket {

yarpl::Reference<yarpl::single::Single<rsocket::Payload>>
RSocketResponder::handleRequestResponse(rsocket::Payload, rsocket::StreamId) {
  return yarpl::single::Singles::error<rsocket::Payload>(
      std::logic_error("handleRequestResponse not implemented"));
}

yarpl::Reference<yarpl::flowable::Flowable<rsocket::Payload>>
RSocketResponder::handleRequestStream(rsocket::Payload, rsocket::StreamId) {
  return yarpl::flowable::Flowables::error<rsocket::Payload>(
      std::logic_error("handleRequestStream not implemented"));
}

yarpl::Reference<yarpl::flowable::Flowable<rsocket::Payload>>
RSocketResponder::handleRequestChannel(
    rsocket::Payload,
    yarpl::Reference<yarpl::flowable::Flowable<rsocket::Payload>>,
    rsocket::StreamId) {
  return yarpl::flowable::Flowables::error<rsocket::Payload>(
      std::logic_error("handleRequestChannel not implemented"));
}

void RSocketResponder::handleFireAndForget(
    rsocket::Payload,
    rsocket::StreamId) {
  // No default implementation, no error response to provide.
}

void RSocketResponder::handleMetadataPush(std::unique_ptr<folly::IOBuf>) {
  // No default implementation, no error response to provide.
}

/// Handles a new Channel requested by the other end.
yarpl::Reference<yarpl::flowable::Subscriber<Payload>>
RSocketResponder::handleRequestChannelCore(
    Payload request,
    StreamId streamId,
    const yarpl::Reference<yarpl::flowable::Subscriber<Payload>>&
        response) noexcept {
  class EagerSubscriberBridge
      : public yarpl::flowable::Subscriber<rsocket::Payload> {
   public:
    void onSubscribe(yarpl::Reference<yarpl::flowable::Subscription>
                         subscription) noexcept override {
      CHECK(!subscription_);
      subscription_ = std::move(subscription);
      if (inner_) {
        inner_->onSubscribe(subscription_);
      }
    }

    void onNext(rsocket::Payload element) noexcept override {
      DCHECK(inner_);
      inner_->onNext(std::move(element));
    }

    void onComplete() noexcept override {
      DCHECK(inner_);
      if (auto inner = std::move(inner_)) {
        inner->onComplete();
        subscription_.reset();
      } else {
        completed_ = true;
      }
    }

    void onError(folly::exception_wrapper ex) noexcept override {
      VLOG(3) << "handleRequestChannelCore::onError: " << ex.what();
      if (auto inner = std::move(inner_)) {
        inner->onError(std::move(ex));
        subscription_.reset();
      } else {
        error_ = std::move(ex);
      }
    }

    void subscribe(
        yarpl::Reference<yarpl::flowable::Subscriber<rsocket::Payload>> inner) {
      CHECK(!inner_); // only one call to subscribe is supported
      CHECK(inner);

      inner_ = std::move(inner);
      if (subscription_) {
        inner_->onSubscribe(subscription_);
        // it's possible to get an error or completion before subscribe happens,
        // delay sending it but send it when this class gets subscribed
        if (completed_) {
          onComplete();
        } else if (error_) {
          onError(std::move(error_));
        }
      }
    }

   private:
    yarpl::Reference<yarpl::flowable::Subscriber<rsocket::Payload>> inner_;
    yarpl::Reference<yarpl::flowable::Subscription> subscription_;
    folly::exception_wrapper error_;
    bool completed_{false};
  };

  auto eagerSubscriber = yarpl::make_ref<EagerSubscriberBridge>();
  auto flowable = handleRequestChannel(
      std::move(request),
      yarpl::flowable::Flowables::fromPublisher<Payload>(
          [eagerSubscriber](
              yarpl::Reference<yarpl::flowable::Subscriber<Payload>>
                  subscriber) { eagerSubscriber->subscribe(subscriber); }),
      std::move(streamId));
  // bridge from the existing eager RequestHandler and old Subscriber type
  // to the lazy Flowable and new Subscriber type
  flowable->subscribe(std::move(response));
  return eagerSubscriber;
}

/// Handles a new Stream requested by the other end.
void RSocketResponder::handleRequestStreamCore(
    Payload request,
    StreamId streamId,
    const yarpl::Reference<yarpl::flowable::Subscriber<Payload>>&
        response) noexcept {
  auto flowable = handleRequestStream(std::move(request), std::move(streamId));
  flowable->subscribe(std::move(response));
}

/// Handles a new inbound RequestResponse requested by the other end.
void RSocketResponder::handleRequestResponseCore(
    Payload request,
    StreamId streamId,
    const yarpl::Reference<yarpl::single::SingleObserver<Payload>>&
        responseObserver) noexcept {
  auto single = handleRequestResponse(std::move(request), streamId);
  single->subscribe(std::move(responseObserver));
}
} // namespace rsocket
