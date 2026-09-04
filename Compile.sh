#!/bin/sh

# Comando de bash para compilar
cd -- "$(dirname -- "$(readlink -f "${BASH_SOURCE[0]}")")"
mkdir build
cd build
cmake ..
make

