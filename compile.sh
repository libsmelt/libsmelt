#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    echo "Usage: $0"
    exit 1
}

[[ -n "$1" ]] || usage
MODEL=$1

set -x

(
    cd inc/
    rm model.h model_defs.h
    ln -s models/$MODEL/model.h .
    ln -s models/$MODEL/model_defs.h .
)
make clean
make -j10
make bench/ab-throughput
make bench/ab-bench

cp bench/ab-bench ab-bench-$MODEL
cp bench/ab-throughput ab-throughput-$MODEL
