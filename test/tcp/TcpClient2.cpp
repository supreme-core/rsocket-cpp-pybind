#include <folly/ExceptionWrapper.h>
#include <folly/SocketAddress.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <reactive-streams/utilities/SmartPointers.h>
#include "src/DuplexConnection.h"
#include "src/Payload.h"
#include "src/ReactiveStreamsCompat.h"
#include "src/mixins/IntrusiveDeleter.h"

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