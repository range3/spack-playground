#!/usr/bin/bash

WORK="/work/NBB/$USER"
JOB_SCRIPT="$WORK/spack-playground/benchmarks/pmemblkbench-job.sh"
LABEL=$(date '+%F_%T')-256G
export OUTPUT_DIR="$WORK/bench_results/pmemblkbench/${LABEL}"
export POOL_SIZE=300G
export TOTAL_SIZE=256G

mkdir -p "$OUTPUT_DIR"
cd $OUTPUT_DIR

access_pattern_list=(
  ""
  "--random"
)

for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16 32; do
for block_size in 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do

qsub \
  -v POOL_SIZE \
  -v TOTAL_SIZE \
  -v OUTPUT_DIR \
  -v ACCESS_PATTERN="${access_pattern}" \
  -v NTHREADS=${nthreads} \
  -v BLOCK_SIZE=${block_size} \
  ${JOB_SCRIPT}

done
done
done
