// Copyright 2004-present Facebook. All Rights Reserved.

#include "StatsPrinter.h"

#include <glog/logging.h>

namespace reactivesocket {
void StatsPrinter::socketCreated() {
  LOG(INFO) << "socketCreated";
}

void StatsPrinter::socketClosed() {
  LOG(INFO) << "socketClosed";
}

    void StatsPrinter::connectionCreated(const std::string &type) {
      LOG(INFO) << "connectionCreated " << type;
    }

    void StatsPrinter::connectionClosed(const std::string &type) {
      LOG(INFO) << "connectionClosed " << type;
    }
}