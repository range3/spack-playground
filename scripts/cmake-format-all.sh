#!/bin/bash

set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"

cd "${PROJECT_ROOT}"
find targets tests \
  -iname "CMakeLists.txt" \
  -o -iname "*.cmake" \
| xargs cmake-format "$@"
