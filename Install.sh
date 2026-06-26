#/bin/sh

cd -- "$(dirname -- "$(readlink -f "${BASH_SOURCE[0]}")")"
cd build
sudo make install
sudo ldconfig

