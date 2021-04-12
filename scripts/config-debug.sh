#!/bin/bash

set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"
cd "${PROJECT_ROOT}"

. ${SCRIPT_DIR}/impl/cmake-config.sh

cmake_config Debug
