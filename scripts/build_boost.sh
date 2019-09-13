#!/bin/bash
export CXX=g++-8
export C=gcc-8
pushd ../../boost_1_71_0
./bootstrap.sh --prefix="${PWD}/../prefix"
./b2 --toolset=gcc-8 threading=multi link=shared variant=release address-model=64 stage 
./b2 install
popd
