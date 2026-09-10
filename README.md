# SoapyIQFile: Mock-up Driver para SoapySDR

Este repositorio contiene un driver virtual para SoapySDR escrito en C++. 
Su función principal es simular un dispositivo SDR de hardware físico en sistemas Linux. 
En lugar de recibir datos desde una antena física, estos se obtienen directamente desde un archivo local o un pipe (FIFO). 
Está diseñado específicamente para procesar muestras en formato 'CF32' (Complex Float 32-bit), obtenidas a partir de un path que recibe como argumento.

## Instalación

Para instalarlo se necesitan herramientas de compilación como `g++`, `make` y `cmake` además del paquete de SoapySDR, primero se corre el script de Bash `Compile.sh`, y luego para instalarlo se ejecuta el script `Install.sh`.

Para probar su correcta instalación se puede usar el utilitario de SoapySDR, `SoapySDRUtil --info`, donde debería salir entre los outputs tanto el módulo, como el factory, un ejemplo de esto siendo:

```
> SoapySDRUtil --info
######################################################
##     Soapy SDR -- the SDR abstraction library     ##
######################################################

Lib Version: v0.8.1-ARCH
API Version: v0.8.0
ABI Version: v0.8
Install root: /usr
Search path:  /usr/lib/SoapySDR/modules0.8                  (missing)
Search path:  /usr/local/lib/SoapySDR/modules0.8
Module found: /usr/local/lib/SoapySDR/modules0.8/libiqfile.so (ef5e55e) # El módulo
Available factories... iqfile # La factory
Available converters...
 -  CF32 -> [CF32, CS16, CS8, CU16, CU8]
 -  CS16 -> [CF32, CS16, CS8, CU16, CU8]
 -  CS32 -> [CS32]
 -   CS8 -> [CF32, CS16, CS8, CU16, CU8]
 -  CU16 -> [CF32, CS16, CS8]
 -   CU8 -> [CF32, CS16, CS8]
 -   F32 -> [F32, S16, S8, U16, U8]
 -   S16 -> [F32, S16, S8, U16, U8]
 -   S32 -> [S32]
 -    S8 -> [F32, S16, S8, U16, U8]
 -   U16 -> [F32, S16, S8]
 -    U8 -> [F32, S16, S8]
```

## Uso

Este driver fue hecho principalmente para permitir a [OpenWebRX+](https://github.com/luarvique/openwebrx) funcionar leyendo de un archivo. Para configurarlo ahí, agregar un dispositivo SoapySDR, e incluirle el parámetro `path=` apuntando al archivo que se leerá, a su vez se pueden agregar las flags `rate`, `freq`, y `repeat` para configurar la velocidad de sampleo, la frecuencia central, y si el archivo se repite luego de terminar (no aplica a archivos FIFO).

---

This repository contains a virtual driver for SoapySDR written in C++ (based on the [PothosWare Example Driver](https://github.com/pothosware/SoapySDR/tree/master/ExampleDriver)). 
It's main purpose is to simulate a SDR driver, but instead of reading data coming from a hardware device, it reads it from a local file, be it an audio file or a pipe file.
At this point it can only process CF32 (Complex Float 32-bit) data.
It works by receiving a `--path=` argument pointing to the file to read.

## Installation

To install the driver, `g++`, `make` and `cmake` are needed, aswell as `soapysdr` itself. With all that, just run the `Compile.sh` and `Install.sh` scripts to install the driver in your Linux system, it will ask for sudo access for the Install.sh as it is needed for both putting the .so file in a lib folder, and adding it to the list with `ldconfig`.

Once it's finished, it should appear in `SoapySDRUtil --info` as shown above.

## Usage

This driver was made to allow [OpenWebRX+](https://github.com/luarvique/openwebrx) to read files instead of SDR sources. So to configure it there, add a SoapySDR device (of device type IQ File / FIFO source) and add the additional option "Device identifier", in there, put the parameter `path` pointing to the file to read (for example "path=/tmp/test.dat"). Other options can be added such as `rate` to change the sample rate, `freq` to configure the center frequency, and `repeat` to set if the file should be looped instead of finishing.
