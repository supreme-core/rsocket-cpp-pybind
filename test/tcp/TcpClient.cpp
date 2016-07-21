#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>

DEFINE_string(host, "localhost", "host to connect to");
DEFINE_int32(port, 9898, "host:port to connect to");


int main(int argc, char* argv[]) {
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  CHECK(FLAGS_host.empty()) << "ahoj vole";
  LOG(INFO) << "smrdis";

  return 0;
}