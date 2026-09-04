# SoapyFile: Mock-up Driver para SoapySDR

Este repositorio contiene un driver virtual para SoapySDR escrito en C++. Su función principal es simular un dispositivo SDR de hardware físico en sistemas Linux. En lugar de recibir datos desde una antena física, estos se obtienen directamente desde un archivo local o un pipe (FIFO). Está diseñado específicamente para procesar muestras en formato 'CF32' (Complex Float 32-bit), obtenidas a partir de un path que recibe como argumento.
