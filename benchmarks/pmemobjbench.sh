#!/usr/bin/bash

OUTPUT_DIR_PATH="$HOME/bench_results/pmemobjbench"
PMEMOBJBENCH_EXE="$HOME/work/spack-playground/build-RelWithDebInfo/bin/pmemobjbench"
POOL_PATH=/mnt/pmem0/$USER/pmemobjbench.pool
# POOL_PATH=/dev/dax0.0
POOL_SIZE=8G
TOTAL_SIZE=4G
TIMESTAMP=$(date +%Y.%m.%d-%H.%M.%S)
LABEL=4G

access_pattern_list=(
  ""
  "--random"
)

access_type_list=(
  "--write"
  "--read"
)

OUTPUT_DIR_PATH="${OUTPUT_DIR_PATH}/${TIMESTAMP}-${LABEL}"
mkdir -p "$OUTPUT_DIR_PATH"

rm -f $POOL_PATH

for access_pattern in "${access_pattern_list[@]}"; do
for nthreads in 1 2 4 8 16; do
for block_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
# block_size_byte=$(echo $block_size | numfmt --from=iec)
# stripe_size=$((block_size_byte * nthreads))
for access_type in "${access_type_list[@]}"; do
for iteration in 0; do

OUTPUT_FILE="${TOTAL_SIZE}_${block_size}_${nthreads}_${access_type}_${access_pattern}_${iteration}"
echo $OUTPUT_FILE

  # --stripe $stripe_size
PMEMOBJ_CONF="prefault.at_open=1;prefault.at_create=1" \
$PMEMOBJBENCH_EXE \
  --path $POOL_PATH \
  --file_size $POOL_SIZE \
  --total $TOTAL_SIZE \
  --stripe $TOTAL_SIZE \
  --block $block_size \
  --nthreads $nthreads \
  $access_type \
  $access_pattern \
  > "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stdout" \
  2> "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stderr"

rm $POOL_PATH

done
done
done
done
done
