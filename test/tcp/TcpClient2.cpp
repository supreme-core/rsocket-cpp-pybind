#include <glog/logging.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/TimeoutManager.h>
#include <folly/io/async/Request.h>
#include <folly/Executor.h>
#include <folly/experimental/ExecutionObserver.h>
#include <folly/futures/DrivableExecutor.h>
#include <boost/intrusive/list.hpp>
#include <boost/utility.hpp>
#include <event.h>  // libevent

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