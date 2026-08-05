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

CMAKE_GENERATOR_FLAGS=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    CMAKE_GENERATOR_FLAGS+="-G Xcode"
fi

mkdir -p build
pushd build > /dev/null

BUILD_DIR=$PWD
cmake $CMAKE_GENERATOR_FLAGS -S $SRC_DIR -B $BUILD_DIR
cmake --build $BUILD_DIR $PARALLEL_BUILD
STATUS=$?

popd > /dev/null

exit $STATUS
