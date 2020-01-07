#!/bin/bash
conan profile new default --detect || true
conan profile update settings.compiler.libcxx=libstdc++11 default || true
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan True || true
conan remote add conan-community https://api.bintray.com/conan/conan-community/conan True || true
conan remote add inexorgame https://api.bintray.com/conan/inexorgame/inexor-conan True || true