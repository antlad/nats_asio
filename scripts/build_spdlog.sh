#!/bin/bash
pushd ../../spdlog-1.3.1
mkdir -p build
pushd build
cmake -DBUILD_SHARED_LIBS=ON -DSPDLOG_BUILD_BENCH=OFF -DSPDLOG_BUILD_TESTS=OFF -DSPDLOG_FMT_EXTERNAL=ON -DSPDLOG_BUILD_EXAMPLES=OFF -DCMAKE_TOOLCHAIN_FILE="${PWD}/../../prefix/toolchain.txt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PWD}/../../prefix" -GNinja ..
ninja install
popd
popd
