#!/bin/bash
pushd ../../openssl-OpenSSL_1_1_1c
export prefix_path="${PWD}/../prefix/"

./Configure linux-generic64 --prefix="${prefix_path}"
make 
cp libcrypto*  "${prefix_path}/lib"
cp libssl* "${prefix_path}/lib"

popd
