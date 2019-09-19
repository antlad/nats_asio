#!/bin/bash
pushd ../../googletest-release-1.8.1

mkdir -p build
pushd build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE="${PWD}/../../prefix/toolchain.txt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PWD}/../../prefix" -GNinja ..
ninja install
popd

popd
