#!/usr/bin/bash

OUTPUT_DIR_PATH="$HOME/bench_results/pmem2bench"
PMEM2BENCH_EXE="$HOME/work/spack-playground/build-RelWithDebInfo/bin/pmem2bench"
POOL_PATH=/mnt/pmem0/$USER/pmem2bench.pool
POOL_SIZE=64G
TOTAL_SIZE=64G
LABEL=$(date '+%F_%T')-64G-non-temporal

access_pattern_list=(
  ""
  "--random"
)

access_type_list=(
  "--write"
  "--read"
)

store_type_list=(
  # ""
  "--non-temporal"
)


OUTPUT_DIR_PATH="${OUTPUT_DIR_PATH}/${LABEL}"
mkdir -p "$OUTPUT_DIR_PATH"

for store_type in "${store_type_list[@]}"; do
for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16; do
for block_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
for access_type in "${access_type_list[@]}"; do
for iteration in 0 1 2; do

OUTPUT_FILE="${TOTAL_SIZE}_${block_size}_${nthreads}_${access_type}_${access_pattern}_${store_type}_${iteration}"
echo $OUTPUT_FILE

$PMEM2BENCH_EXE \
  --file $POOL_PATH \
  --file_size $POOL_SIZE \
  --total $TOTAL_SIZE \
  --stripe $TOTAL_SIZE \
  --block $block_size \
  --nthreads $nthreads \
  $access_type \
  $access_pattern \
  $store_type \
  > "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stdout" \
  2> "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stderr"

done
done

rm $POOL_PATH

done
done
done
done
