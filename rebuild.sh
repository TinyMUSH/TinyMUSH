#!/bin/bash
killall -9 netmush
rm -rf ./build
rm -rf ./game
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --target install-upgrade -j$(nproc)

