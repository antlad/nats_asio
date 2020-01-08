# nats-asio

## Overview
This is [nats-io](https://nats.io/) client writen in c++14 with use of [boost](https://www.boost.org/) [asio](https://www.boost.org/doc/libs/release/libs/asio/) and corutines libraries.

## Requirements
- boost - connectivity and async work (using asio and corutines, tested on 1.71)
- openssl - inderect of asio when using ssl connection (tested 1.1.1)
- fmtlib/fmt@6.0.0
- gabime/spdlog@v1.4.2
- nlohmann/json@v3.7.0
- google/googletest@release-1.10.0 (ENABLE_TESTS off by default)
- cxxopts@v2.2.0 (ENABLE_EXAMPLES off by default)

## Build 
You can manually start conan install in build folder (`conan install --build=missing ..`). But if you try to open project under something like QtCreator that is trying cmake in temp folder add -DENABLE_CONAN_QTCREATOR_SUPPORT=ON flag to cmake
```bash
./config_conan.sh
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=${PREFIX}  -DCMAKE_INSTALL_PREFIX=${PREFIX} ${CMAKE_ARGS}
cmake --build .
cmake --build . --target install
```
