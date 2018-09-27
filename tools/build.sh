#!/bin/sh
set -ex
mkdir -p build
cd build
cmake ..
make -j all test
