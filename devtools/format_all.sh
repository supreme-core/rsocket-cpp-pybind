#!/usr/bin/env bash
set -xue

cd "$(dirname "$0")/.."
find src/ -type f '(' -name '*.cpp' -o -name '*.h' ')' -exec clang-format -i {} \;

# EOF
