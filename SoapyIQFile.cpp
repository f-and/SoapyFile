#include "SoapyIQFile.hpp"
#include <SoapySDR/Registry.hpp>

// El constructor SoapyIQFile crea un device.
/* Lee los argumentos representativos de un SDR: sampleRate, centerFreq (frecuencia central), repeat (un booleano que indica si el archivo debe 
repetirse luego de su finalización), isFifo (revisa que el modo de lectura sea por pipe), samplesDelivered (la cantidad de muestras leídas), 
residualLength (la longitud residual luego de la lectura) y, principalmente, un path.
*/
// El path representa al archivo del que se va a leer, en lugar del SDR real.
SoapyIQFile::SoapyIQFile(const SoapySDR::Kwargs &args): 
    _path(""), _sampleRate(1.0), _centerFreq(0.0), _repeat(false), _isFifo(false), _samplesDelivered(0), _residualLength(0)  
    { 
        if (args.count("path") > 0) { _path = args.at("path"); } // Verifica que obligatoriamente exista un archivo del cual leer
        else { throw std::runtime_error("Path argument is required, but wasn't found."); }
        if (args.count("rate") > 0) { _sampleRate = std::stod(args.at("rate")); } 
        if (args.count("freq") > 0) { _centerFreq = std::stod(args.at("freq")); } 
        if (args.count("repeat") > 0 && args.at("repeat") == "true") { _repeat = true; }
        struct stat fileStat;
        stat(_path.c_str(), &fileStat); // Abre el archivo con stat para poder verificar su status
        if (S_ISFIFO(fileStat.st_mode)) { _isFifo = true; }
    }

std::string SoapyIQFile::getDriverKey(void) const { return "iqfile"; }

std::string SoapyIQFile::getHardwareKey(void) const { return "iqfile"; }

// Brinda información del entorno de acuerdo a lo setteado
SoapySDR::Kwargs SoapyIQFile::getHardwareInfo(void) const
{
    SoapySDR::Kwargs args = SoapySDR::Kwargs();
    args["path"] = _path;
    args["rate"] = _sampleRate;
    args["freq"] = _centerFreq;
    return args;
}

std::vector<std::string> SoapyIQFile::getStreamFormats (const int direction, const size_t channel) const 
{ 
    return (direction == SOAPY_SDR_RX) ? std::vector<std::string>{SOAPY_SDR_CF32} : std::vector<std::string>{}; 
} 

std::string SoapyIQFile::getNativeStreamFormat (const int direction, const size_t channel, double &fullScale) const 
{ 
    return (direction == SOAPY_SDR_RX) ? SOAPY_SDR_CF32 : ""; 
} 

size_t SoapyIQFile::getNumChannels (const int direction) const 
{
    return (direction == SOAPY_SDR_RX) ? 1 : 0; 
}

// Define el formato (junto con algunas restricciones) y abre el archivo en modo binario
SoapySDR::Stream* SoapyIQFile::setupStream (const int direction, const std::string &format, const std::vector<size_t> &channels = std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs()) 
{
    if (direction != SOAPY_SDR_RX) { throw std::runtime_error("TX SoapySDR::Stream was asked, but the only channel available is RX."); } // Impide transmisiones (Tx), dado que funciona sólo como receptor (Rx)
    if (format  != SOAPY_SDR_CF32) { throw std::runtime_error("Asked format is not available, the only format currently implemented is CF32."); } //Obliga a que el formato sea Complex Float 32bit, dado que es lo que devuelve GNU Radio

    _file.open(_path, std::ios::binary);
    if (!_file.is_open()) { throw std::runtime_error("Tried to open file " + _path + ", but it failed to open with error: \"" + std::strerror(errno) + "\"."); }

    return (SoapySDR::Stream *)(this);
}

void SoapyIQFile::closeStream (SoapySDR::Stream *stream) 
{
    _file.close();
    return;
}

int SoapyIQFile::activateStream (SoapySDR::Stream *stream, const int flags=0, const long long timeNs=0, const size_t numElems=0) 
{
    _startTime = std::chrono::steady_clock::now();
    _samplesDelivered = 0;
    if ((flags & (1 << 2)) != 0 && timeNs != 0) { return SOAPY_SDR_NOT_SUPPORTED; }
    return 0;
}

int SoapyIQFile::deactivateStream (SoapySDR::Stream *stream, const int flags=0, const long long timeNs=0)
{
    if ((flags & (1 << 2)) != 0 && timeNs != 0) { return SOAPY_SDR_NOT_SUPPORTED; }
    return 0;
}

// Concentra la lógica principal de la lectura del archivo
int SoapyIQFile::readStream (SoapySDR::Stream *stream, void *const *buffs, const size_t numElems, int &flags, long long &timeNs, const long timeoutUs=100000)
{
    // Prepara la salida
    float *out = (float *)buffs[0];
    size_t numberOfComplexSamples = numElems * 2; // Duplica el número de muestras dada la naturaleza compleja de las mismas
    size_t bytesAsked = numberOfComplexSamples * sizeof(float); // Calcula la cantidad de bytes pedida desde la última lectura (donde quedó el puntero)

    // Maneja un residual en caso de que la lectura del pipe se corte antes de su finalización (verifica que no hayan faltantes)
    if (_residualLength > 0) 
    {
        std::memcpy(reinterpret_cast<char *>(out), _residual, _residualLength);
    }

    _file.read(reinterpret_cast<char *>(out + _residualLength), bytesAsked - _residualLength);
    std::streamsize bytesRead = _residualLength + _file.gcount();
    if (_file.eof() || _file.fail()) 
    {
        _file.clear(); // Si terminó el archivo o hubo una falla, limpia todo
    }
    const size_t bytesPerSample = 2 * sizeof(float);
    size_t samplesRead = bytesRead / bytesPerSample; // Compara la cantidad leída con la cantidad esperada por muestra
    size_t bytesRemaining = bytesRead % bytesPerSample; // Verifica la existencia de un remantente, que luego es asignado al residual, para tenerlos en cuenta en la próxima lectura

    // En caso de haber leído de más, almacena ese residual para la próxima lectura. En caso de haber leído menos de la cantidad esperada, deberá traerlas en la próxima lectura

    _residualLength = bytesRemaining;
    if (bytesRemaining > 0)
    {
        std::memcpy(_residual, reinterpret_cast<char *>(out + samplesRead * bytesPerSample), _residualLength);
    }

    // Si el pipe se vacía pero no se cerró (no llegó al EOF), avisa que hubo un timeout para que OpenWebRX no se cuelgue esperando.
    if (samplesRead == 0 && !(_file.eof()))
    {
        return SOAPY_SDR_TIMEOUT;
    }

    /* Simulación de Tiempo Real: Se utiliza un sleep para evitar que el archivo se lea a la velocidad máxima del procesador,
    sino que simule el comportamiento de una antena (que lee a la velocidad del Sample Rate) */
    double totalExpectedSeconds = double(_samplesDelivered) / _sampleRate;
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    double totalRealSeconds = std::chrono::duration<double>(now - _startTime).count();

    if (totalExpectedSeconds > totalRealSeconds && _isFifo)
    {
        double sleepTime = totalExpectedSeconds - totalRealSeconds;
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
    }

    _samplesDelivered += int(samplesRead);
    return int(samplesRead);

}

void SoapyIQFile::setSampleRate (const int direction, const size_t channel, const double rate)
{
    _sampleRate = rate;
}

double SoapyIQFile::getSampleRate (const int direction, const size_t channel) const
{
    return _sampleRate;
}

std::vector<double> SoapyIQFile::listSampleRates (const int direction, const size_t channel) const
{
    return std::vector<double>{_sampleRate};
}

SoapySDR::RangeList SoapyIQFile::getSampleRateRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_sampleRate, _sampleRate)};
}

void SoapyIQFile::setFrequency (const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args=SoapySDR::Kwargs())
{
    _centerFreq = frequency;
}

double SoapyIQFile::getFrequency (const int direction, const size_t channel) const
{
    return _centerFreq;
}

std::vector<std::string> SoapyIQFile::listFrequencies (const int direction, const size_t channel) const
{
    return std::vector<std::string>{"RF"};
}

SoapySDR::RangeList SoapyIQFile::getFrequencyRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_centerFreq - _sampleRate/2, _centerFreq + _sampleRate/2)};
}

std::vector<std::string> SoapyIQFile::listGains (const int direction, const size_t channel) const
{
    return std::vector<std::string>{};
}

bool SoapyIQFile::hasGainMode (const int direction, const size_t channel) const
{
    return false;
}

void SoapyIQFile::setGain (const int direction, const size_t channel, const double value)
{
    return;
}

double SoapyIQFile::getGain (const int direction, const size_t channel) const
{
    return 0;
}

SoapySDR::Range SoapyIQFile::getGainRange (const int direction, const size_t channel) const
{
    return SoapySDR::Range(0, 0);
}

std::vector<std::string> SoapyIQFile::listAntennas (const int direction, const size_t channel) const
{
    return std::vector<std::string>{"RX"};
}

void SoapyIQFile::setAntenna (const int direction, const size_t channel, const std::string &name)
{
    return;
}

std::string SoapyIQFile::getAntenna (const int direction, const size_t channel) const
{
    return "RX";
}

void SoapyIQFile::setBandwidth (const int direction, const size_t channel, const double bw)
{
    _sampleRate = bw;
}

double SoapyIQFile::getBandwidth (const int direction, const size_t channel) const
{
    return _sampleRate;
}

std::vector<double> SoapyIQFile::listBandwidths (const int direction, const size_t channel) const
{
    return std::vector<double>{_sampleRate};
}

SoapySDR::RangeList SoapyIQFile::getBandwidthRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_sampleRate, _sampleRate)};
}

/***********************************************************************
 * Find available devices
 **********************************************************************/
// Busca un driver llamado "IQ File"
static SoapySDR::KwargsList findIQFile(const SoapySDR::Kwargs &args)
{
    SoapySDR::KwargsList results;

    if  (args.count("driver") > 0 && args.at("driver") == "iqfile")
    {
        SoapySDR::Kwargs device;
        device["label"] = "IQ File Source";
        if (args.count("path") > 0) { device["path"] = args.at("path"); }
        results.push_back(device);
    }

    return results;    
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeIQFile(const SoapySDR::Kwargs &args)
{
    return new SoapyIQFile(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerFile("iqfile", &findIQFile, &makeIQFile, SOAPY_SDR_ABI_VERSION);
