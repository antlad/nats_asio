#!/bin/bash
export CGET_PREFIX=`realpath $PWD/../../prefix2`
cget init --toolchain ${CGET_PREFIX}/toolchain.cmake
cget install pfultz2/cget-recipes
cget install fmtlib/fmt@6.0.0
cget install gabime/spdlog@v1.4.2
cget install google/googletest@release-1.10.0
cget install jarro2783/cxxopts@v2.2.0

