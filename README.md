# reactivesocket-cpp
C++ implementation of ReactiveSocket

# Build Status

<a href='https://travis-ci.org/ReactiveSocket/reactivesocket-cpp/builds'><img src='https://travis-ci.org/ReactiveSocket/reactivesocket-cpp.svg?branch=master'></a>

# Building and running tests

After installing latest folly release with `brew install folly` and making sure that you've checked out external dependencies via `git submodule update --recursive`, you can build and run tests with:

```
  mkdir -p build
  cd build
  cmake ../
  make
  ./ReactiveSocketTest
```
