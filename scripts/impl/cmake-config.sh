function cmake_config() {
  BUILD_TYPE=${1:-RelWithDebInfo}
  PROFILE=${2:-default}

  case "$PROFILE" in
    "clang" )
      CC=clang CXX=clang++ cmake -S . -B build-${BUILD_TYPE} -DCMAKE_BUILD_TYPE=${BUILD_TYPE};;
    *)
      cmake -S . -B build-${BUILD_TYPE} -DCMAKE_BUILD_TYPE=${BUILD_TYPE};;
  esac
}
