def benchmark(name, src):
  cpp_benchmark(
    name = name,
    srcs = [
      src,
      "benchmarks/Benchmarks.cpp",
      "benchmarks/Fixture.cpp",
    ],
    deps = [
      ":benchmark-headers",
      ":rsocket-tcp",
      "//folly:benchmark",
      "//folly/init:init",
    ],
  )
