#!/bin/bash

function pretty_to_byte {
  numfmt --from=iec $1
}

MPIRUN_EXE=mpirun
IOR_EXE=$HOME/work/io500/bin/ior
OUTPUT_DIR_PATH="$HOME/bench_results/ior"
POOL_PATH=/mnt/pmem0/$USER/ior_file
POOL_SIZE=8G
TOTAL_SIZE=${POOL_SIZE}
LABEL=$(date --iso-8601=seconds)-8G


option_file_per_procs_list=(
  ""
  "-F"
)

access_type_list=(
  "-w"
  "-r"
)

OUTPUT_DIR_PATH="${OUTPUT_DIR_PATH}/${LABEL}"
mkdir -p "$OUTPUT_DIR_PATH"

total_size_byte=$(pretty_to_byte $TOTAL_SIZE)

for nprocs in 1 2 4 8 16; do
# for nprocs in 4; do
for segment_count in 1; do
for option_file_per_procs in "${option_file_per_procs_list[@]}"; do
for transfer_size in 64 128 256 512 1K 2K 4K 8K 16K 32K 64K 128K 256K 512K 1M; do
# for transfer_size in 1M; do
block_size=$((total_size_byte / segment_count / nprocs))

for access_type in "${access_type_list[@]}"; do
for iteration in 0 1 2; do

block_size_pretty=$(numfmt --to=iec ${block_size})
OUTPUT_FILE="${TOTAL_SIZE}_${segment_count}_${block_size_pretty}_${transfer_size}_${nprocs}_${access_type}_${option_file_per_procs}_${iteration}"
echo $OUTPUT_FILE

$MPIRUN_EXE \
  -x PATH \
  -x LD_LIBRARY_PATH \
  -np $nprocs \
$IOR_EXE \
  -C \
  -Q 1 \
  -g \
  -G 271 \
  -k \
  -e \
  -o $POOL_PATH \
  -b $block_size \
  -t $transfer_size \
  $option_file_per_procs \
  $access_type \
  > "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stdout" \
  2> "${OUTPUT_DIR_PATH}/${OUTPUT_FILE}.stderr"

# drop page cache
sudo sh -c "echo 1 > /proc/sys/vm/drop_caches"

done
done
rm ${POOL_PATH}*
done
done
done
done
