#/bin/sh

# Comando de bash para instalar
cd -- "$(dirname -- "$(readlink -f "${BASH_SOURCE[0]}")")"
cd build
sudo make install
sudo ldconfig

