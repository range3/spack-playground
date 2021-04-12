#!/bin/bash

set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"

cd "${PROJECT_ROOT}"
find targets tests \
  -iname "*.cpp" \
  -o -iname "*.cc" \
  -o -iname "*.cxx" \
  -o -iname "*.c++" \
  -o -name "*.C" \
  -o -iname "*.hpp" \
  -o -iname "*.h" \
| xargs clang-format -i
