#!/bin/sh

ROOT_DIR=$PWD
SRC_DIR=$ROOT_DIR/src

# flags
PARALLEL_BUILD=""
for arg in "$@"; do
    if [ "$arg" = "--parallel" ]; then
        PARALLEL_BUILD="--parallel"
    fi
done

mkdir -p build
pushd build > /dev/null

BUILD_DIR=$PWD
cmake -S $SRC_DIR -B $BUILD_DIR
cmake --build $BUILD_DIR $PARALLEL_BUILD

popd > /dev/null
