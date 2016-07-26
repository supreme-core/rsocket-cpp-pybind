// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

#include <gmock/gmock.h>
#include <src/Stats.h>
#include <src/tcp/TcpDuplexConnection.h>

#include "src/Payload.h"

namespace reactivesocket {

class MockStats : public Stats {
 public:
    MOCK_METHOD0(socketCreated_, void());
    MOCK_METHOD0(socketClosed_, void());
    MOCK_METHOD1(connectionCreated_, void(const std::string&));
    MOCK_METHOD1(connectionClosed_, void(const std::string&));

  void socketCreated() override {
    socketCreated_();
  }

  void socketClosed() override {
    socketClosed_();
  }

    void connectionCreated(const char type[4], reactivesocket::TcpDuplexConnection *pConnection) override {
      connectionCreated_(type);
    }

    void connectionClosed(const char type[4], reactivesocket::TcpDuplexConnection *pConnection) override {
       connectionClosed_(type);
      }
};
}
