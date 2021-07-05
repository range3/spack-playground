#!/usr/bin/bash

OUTPUT_DIR_PATH="$HOME/bench_results/pmemblkbench"
PMEMBLKBENCH_EXE="$HOME/work/spack-playground/build-RelWithDebInfo/bin/pmemblkbench"
POOL_PATH=/mnt/pmem0/$USER/pmemblkbench.pool
POOL_SIZE=100G
TOTAL_SIZE=64G
LABEL=$(date --iso-8601=seconds)-64G

access_pattern_list=(
  ""
  "--random"
)

access_type_list=(
  "--write"
  "--read"
)

OUTPUT_DIR_PATH="${OUTPUT_DIR_PATH}/${LABEL}"
mkdir -p "$OUTPUT_DIR_PATH"

for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16 32; do
for block_size in 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
for access_type in "${access_type_list[@]}"; do
for iteration in 0 1 2; do

OUTPUT_FILE="${TOTAL_SIZE}-${TOTAL_SIZE}-${block_size}-${nthreads}-${access_type}-${access_pattern}-${iteration}"
echo $OUTPUT_FILE

$PMEMBLKBENCH_EXE \
  --pool $POOL_PATH \
  --pool_size $POOL_SIZE \
  --total $TOTAL_SIZE \
  --stripe $TOTAL_SIZE \
  --block $block_size \
  --nthreads $nthreads \
  $access_type \
  $access_pattern \
  > "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stdout" \
  2> "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stderr"

done
done

rm $POOL_PATH

done
done
done
