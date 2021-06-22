#!/bin/bash
#------------qsub option------------
#PBS -A NBB
#PBS -q pmem-b
#PBS -l elapstim_req=24:00:00
#PBS -v OMP_NUM_THREADS=32
#------- Program execution -----------

: ${WORK:="/work/NBB/$USER"}
: ${PMEMBLKBENCH_EXE:="$WORK/spack-playground/build-RelWithDebInfo/bin/pmemblkbench"}
: ${POOL_PATH:="/pmem0/pmemblkbench.pool"}
: ${POOL_SIZE:=300G}
: ${TOTAL_SIZE:=256G}
: ${STRIPE_SIZE:=$TOTAL_SIZE}
: ${BLOCK_SIZE:=512}
: ${NTHREADS:=1}
: ${ACCESS_PATTERN:=""}
: ${OUTPUT_DIR:="$WORK/bench_results/pmemblkbench"}

access_type_list=(
  "--write"
  "--read"
)

module load pmdk/20210401

for access_type in "${access_type_list[@]}"; do
for iteration in 0 1 2; do

output_file="${POOL_SIZE}_${TOTAL_SIZE}_${STRIPE_SIZE}_${BLOCK_SIZE}_${NTHREADS}_${ACCESS_PATTERN}_${access_type}_${iteration}"
echo ${output_file}

$PMEMBLKBENCH_EXE \
  --pool $POOL_PATH \
  --pool_size $POOL_SIZE \
  --total $TOTAL_SIZE \
  --stripe $STRIPE_SIZE \
  --block $BLOCK_SIZE \
  --nthreads $NTHREADS \
  $access_type \
  $ACCESS_PATTERN \
  > "${OUTPUT_DIR}/${output_file}.stdout" \
  2> "${OUTPUT_DIR}/${output_file}.stderr"

done
done

rm $POOL_PATH
