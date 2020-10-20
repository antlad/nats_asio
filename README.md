# nats-asio

## Overview
This is [nats-io](https://nats.io/) client writen in c++14 with use of [boost](https://www.boost.org/) [asio](https://www.boost.org/doc/libs/release/libs/asio/) and coroutines libraries.

## Requirements
For Library
```
boost/1.71.0@conan/stable
fmt/6.1.2
spdlog/1.5.0
OpenSSL/1.1.1c@conan/stable
jsonformoderncpp/3.7.2@vthiery/stable
```

For tests 
```
gtest/1.8.1@bincrafters/stable
```
For nats tool 
```
cxxopts/v2.1.2@inexorgame/stable
```

## Usage of library
 - You can just copy `interface.hpp` and `impl.hpp` in you project (don't forget to include `impl.hpp` somewhere)
 - Or you can use with conan package. Add conan remote:
```bash
conan remote add antlad-conan https://api.bintray.com/conan/antlad/antlad-conan
```
And then add `nats_asio/0.0.11@_/_` to dependencies. 

If you use 17 standard, don't forget to specify it in conan profile or during install, more details [here]( https://docs.conan.io/en/1.7/howtos/manage_cpp_standard.html)

## Example
Please check source code of tool `samples/nats_tool.cpp`
