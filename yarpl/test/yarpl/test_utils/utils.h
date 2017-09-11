#pragma once

namespace yarpl {
namespace test_utils {

auto const default_baton_timeout = std::chrono::milliseconds(100);
#define CHECK_WAIT(baton) \
  CHECK(baton.timed_wait(::yarpl::test_utils::default_baton_timeout))
}
}
