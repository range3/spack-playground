#!/usr/bin/bash

WORK="/work/NBB/$USER"
JOB_SCRIPT="$WORK/spack-playground/benchmarks/pmem2bench-job.sh"
LABEL=$(date '+%F_%T')-256G-numa
export OUTPUT_DIR="$WORK/bench_results/pmem2bench/${LABEL}"
export POOL_SIZE=256G
export TOTAL_SIZE=256G

mkdir -p "$OUTPUT_DIR"
cd $OUTPUT_DIR

access_pattern_list=(
  ""
  # "--random"
)

store_type_list=(
  ""
  # "--non-temporal"
)

for store_type in "${store_type_list[@]}"; do
for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16; do
for block_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
for numa_node in 0 1; do

qsub \
  -v NUMA_NODE=${numa_node} \
  -v POOL_SIZE \
  -v TOTAL_SIZE \
  -v OUTPUT_DIR \
  -v ACCESS_PATTERN="${access_pattern}" \
  -v STORE_TYPE="${store_type}" \
  -v NTHREADS=${nthreads} \
  -v BLOCK_SIZE=${block_size} \
  ${JOB_SCRIPT}

done
done
done
done
done
