#!/bin/bash

set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"

cd "${PROJECT_ROOT}"
curl -Lo cmake/CPM.cmake https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
