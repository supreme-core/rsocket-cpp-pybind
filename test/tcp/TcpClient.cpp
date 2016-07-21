#include <thread>
#include <folly/Memory.h>
#include <gmock/gmock.h>
#include "src/ReactiveSocket.h"
#include "src/RequestHandler.h"
#include "src/framed/FramedDuplexConnection.h"
#include "src/mixins/MemoryMixin.h"
#include "test/simple/CancelSubscriber.h"
#include "src/tcp/TcpDuplexConnection.h"
#include "test/simple/NullSubscription.h"
#include "test/simple/PrintSubscriber.h"

using namespace ::testing;
using namespace ::reactivesocket;
using namespace ::folly;

DEFINE_string(host, "localhost", "host to connect to");
DEFINE_int32(port, 9898, "host:port to connect to");


int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  CHECK(FLAGS_host.empty()) << "ahoj vole";
  LOG(INFO) << "smrdis";

  return 0;
}