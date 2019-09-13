#!/bin/bash
pushd ../../fmt-6.0.0
mkdir -p build
pushd build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE="${PWD}/../../prefix/toolchain.txt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PWD}/../../prefix" -GNinja ..
ninja install
popd
popd
