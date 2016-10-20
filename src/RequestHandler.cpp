// Copyright 2004-present Facebook. All Rights Reserved.

#include "src/RequestHandler.h"

namespace reactivesocket {

Subscriber<Payload>& RequestHandler::handleRequestChannel(
    Payload request,
    SubscriberFactory& subscriberFactory) {
  return handleRequestChannel(
      std::move(request), subscriberFactory.createSubscriber());
}

/// Handles a new Stream requested by the other end.
void RequestHandler::handleRequestStream(
    Payload request,
    SubscriberFactory& subscriberFactory) {
  handleRequestStream(std::move(request), subscriberFactory.createSubscriber());
}

/// Handles a new inbound Subscription requested by the other end.
void RequestHandler::handleRequestSubscription(
    Payload request,
    SubscriberFactory& subscriberFactory) {
  handleRequestSubscription(
      std::move(request), subscriberFactory.createSubscriber());
}

/// Handles a new inbound RequestResponse requested by the other end.
void RequestHandler::handleRequestResponse(
    Payload request,
    SubscriberFactory& subscriberFactory) {
  handleRequestResponse(
      std::move(request), subscriberFactory.createSubscriber());
}
}
