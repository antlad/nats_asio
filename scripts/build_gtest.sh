#!/bin/bash
pushd ../../googletest-release-1.8.0
pushd googletest
    mkdir -p build
	pushd build
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE="${PWD}/../../../fincharts/toolchain_gcc.txt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PWD}/../../../prefix" -GNinja ..
	ninja install
	popd
popd
pushd googlemock
    mkdir -p build
	pushd build
	cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_TOOLCHAIN_FILE="${PWD}/../../../fincharts/toolchain_gcc.txt" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${PWD}/../../../prefix" -GNinja ..
	ninja install
	popd
popd

popd
