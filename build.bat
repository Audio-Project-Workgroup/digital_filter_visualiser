@echo off

set ROOT_DIR=%CD%
set SRC_DIR=%ROOT_DIR%\src

if not exist build mkdir build
pushd build

set BUILD_DIR=%CD%
cmake -S %SRC_DIR% -B %BUILD_DIR%
cmake --build %BUILD_DIR% --parallel

popd
