#!/bin/bash
conan profile new default --detect || true
conan profile update settings.compiler.libcxx=libstdc++11 default || true
conan remote add inexorgame https://api.bintray.com/conan/inexorgame/inexor-conan True || true
