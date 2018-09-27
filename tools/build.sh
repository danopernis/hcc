#!/bin/sh
# Copyright (c) 2012-2018 Dano Pernis

set -ex
mkdir -p build
cd build
cmake ..
make -j all test
