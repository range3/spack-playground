#!/usr/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$( dirname "${SCRIPT_DIR}" )"


build_prefix="${PROJECT_ROOT}/build-RelWithDebInfo"
bin_path="${build_prefix}/bin"
thallium_rdma_exe="${bin_path}/thallium_rdma2"


addrs=(
  "sockets"
  "tcp"
)

payloadSizes=(
  1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 1048576
)

# set -x

for addr in "${addrs[@]}"; do
  for size in "${payloadSizes[@]}"; do
    # echo $addr
    # echo $size

    # $thallium_rdma_exe -h

    ${thallium_rdma_exe} -S -a ${addr} -s $size > /tmp/addr &
    server_pid=$!
    sleep 1
    for iter in {1..3}; do
      ${thallium_rdma_exe} -a "$(cat /tmp/addr)" -s $size
    done
    kill $server_pid
    wait $server_pid
    tail -n 3 /tmp/addr

  done
done
