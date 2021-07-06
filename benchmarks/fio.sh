#!/bin/bash

FIO_EXE=fio
OUTPUT_DIR_PATH="$HOME/bench_results/fio"
# POOL_PATH=/mnt/pmem0/$USER/fio_file
POOL_PATH=./fio_file
POOL_SIZE=8G
TOTAL_SIZE=${POOL_SIZE}
LABEL=$(date --iso-8601=seconds)-8G-libpmem

$FIO_EXE \
  -filename=$POOL_PATH \
  -rw=read \
  -bs=4K \
  -size=1M \
  -numjobs=1 \
  -runtime=10000000000 \
  -group_reporting \
  -name=seq_read
