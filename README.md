# nats-asio

## Overview
This is [nats-io](https://nats.io/) client writen in c++14 with use of [boost](https://www.boost.org/) [asio](https://www.boost.org/doc/libs/release/libs/asio/) and corutines libraries.

## Requirements

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
