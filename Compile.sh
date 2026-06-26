#!/bin/sh

cd -- "$(dirname -- "$(readlink -f "${BASH_SOURCE[0]}")")"
mkdir build
cd build
cmake ..
make

