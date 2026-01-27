#!/bin/sh

ROOT_DIR=$PWD
SRC_DIR=$ROOT_DIR/src

mkdir -p build
pushd build > /dev/null

BUILD_DIR=$PWD
cmake -S $SRC_DIR -B $BUILD_DIR
cmake --build $BUILD_DIR --parallel

popd > /dev/null
