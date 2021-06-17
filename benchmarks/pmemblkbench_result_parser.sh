#!/usr/bin/bash

OUTPUT_DIR_PATH="$HOME/bench_results/pmemblkbench"

# cat "$@" | jq -s 'sort_by(.params.blockSize) | sort_by(.parmas.nthreads) | .[].params' 
# cat "$@" | jq -s 'sort_by(.parmas.nthreads) | .[].params | select(.accessType == "write")' 

accessType="write"
accessPattern="sequencial"

for nthreads in 1 2 4 8 16 32; do
cat "$@" | jq -s "sort_by(.parmas.blockSize) | .[] | select(.params.nthreads==${nthreads}) | select(.params.accessType=\"${accessType}\") | select(.params.accessPattern=\"${accessPattern}\")" 
done
