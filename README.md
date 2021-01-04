# nats-asio

## Overview
This is [nats-io](https://nats.io/) client writen in c++14 with use of [boost](https://www.boost.org/) [asio](https://www.boost.org/doc/libs/release/libs/asio/) and coroutines libraries.

## Requirements
For Library
```
boost/1.74.0
fmt/6.2.0
spdlog/1.5.0
openssl/1.1.1d
nlohmann_json/3.9.1
```

For tests 
```
gtest/1.8.1
```
For nats tool 
```
cxxopts/2.2.1
```

## Usage of library
 - You can just copy `interface.hpp` and `impl.hpp` in you project (don't forget to include `impl.hpp` somewhere)
 - Or you can use with conan package. Add conan remote:
```bash
conan remote add antlad-conan https://api.bintray.com/conan/antlad/antlad-conan
```
And then add `nats_asio/0.0.12@_/_` to dependencies. 

If you use 17 standard, don't forget to specify it in conan profile or during install, more details [here]( https://docs.conan.io/en/1.7/howtos/manage_cpp_standard.html)

## Example
Please check source code of tool `samples/nats_tool.cpp`
