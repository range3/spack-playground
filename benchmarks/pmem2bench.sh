#!/usr/bin/bash

OUTPUT_DIR_PATH="$HOME/bench_results/pmem2bench"
PMEM2BENCH_EXE="sudo $HOME/work/spack-playground/build-RelWithDebInfo/bin/pmem2bench"
SOURCE_TYPE="devdax"
# POOL_PATH=/mnt/pmem0/$USER/pmem2bench.pool
POOL_PATH=/dev/dax0.0
POOL_SIZE=8G
TOTAL_SIZE=8G
LABEL=$(date --iso-8601=seconds)-8G-devdax-tls_buf

access_pattern_list=(
  ""
  "--random"
)

access_type_list=(
  "--write"
  "--read"
)

store_type_list=(
  ""
  "--non-temporal"
)


OUTPUT_DIR_PATH="${OUTPUT_DIR_PATH}/${LABEL}"
mkdir -p "$OUTPUT_DIR_PATH"

for store_type in "${store_type_list[@]}"; do
for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16; do
# for nthreads in 16; do
for block_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
# block_size_byte=$(echo $block_size | numfmt --from=iec)
# stripe_size=$((block_size_byte * nthreads))
for access_type in "${access_type_list[@]}"; do
for iteration in 0 1 2; do

OUTPUT_FILE="${TOTAL_SIZE}_${block_size}_${nthreads}_${access_type}_${access_pattern}_${store_type}_${iteration}"
echo $OUTPUT_FILE

  # --stripe $stripe_size
$PMEM2BENCH_EXE \
  --source $SOURCE_TYPE \
  --path $POOL_PATH \
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

if [ $SOURCE_TYPE = "fsdax" ] ; then
rm $POOL_PATH
fi

done
done
done
done
