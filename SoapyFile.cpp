#include "SoapyFile.hpp"
#include <SoapySDR/Registry.hpp>

SoapyFile::SoapyFile(const SoapySDR::Kwargs &args):
    _path(""), _sampleRate(1.0), _centerFreq(0.0), _repeat(false), _isFifo(false), _samplesDelivered(0), _residualLength(0)
    { 
        if (args.count("path") > 0) { _path = args.at("path"); } 
        else { throw std::runtime_error("Path argument is required, but wasn't found."); }
        if (args.count("rate") > 0) { _sampleRate = std::stod(args.at("rate")); } 
        if (args.count("freq") > 0) { _centerFreq = std::stod(args.at("freq")); } 
        if (args.count("repeat") > 0 && args.at("repeat") == "true") { _repeat = true; }
        struct stat fileStat;
        stat(_path.c_str(), &fileStat);
        if (S_ISFIFO(fileStat.st_mode)) { _isFifo = true; }
    }

std::string SoapyFile::getDriverKey(void) const { return "file"; }

std::string SoapyFile::getHardwareKey(void) const { return "file"; }

SoapySDR::Kwargs SoapyFile::getHardwareInfo(void) const
{
    SoapySDR::Kwargs args = SoapySDR::Kwargs();
    args["path"] = _path;
    args["rate"] = _sampleRate;
    args["freq"] = _centerFreq;
    return args;
}

std::vector<std::string> SoapyFile::getStreamFormats (const int direction, const size_t channel) const 
{ 
    return (direction == SOAPY_SDR_RX) ? std::vector<std::string>{SOAPY_SDR_CF32} : std::vector<std::string>{}; 
} 

std::string SoapyFile::getNativeStreamFormat (const int direction, const size_t channel, double &fullScale) const 
{ 
    return (direction == SOAPY_SDR_RX) ? SOAPY_SDR_CF32 : ""; 
} 

size_t SoapyFile::getNumChannels (const int direction) const 
{
    return (direction == SOAPY_SDR_RX) ? 1 : 0; 
}

SoapySDR::Stream* SoapyFile::setupStream (const int direction, const std::string &format, const std::vector<size_t> &channels = std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs()) 
{
    if (direction != SOAPY_SDR_RX) { throw std::runtime_error("TX SoapySDR::Stream was asked, but the only channel available is RX."); }
    if (format  != SOAPY_SDR_CF32) { throw std::runtime_error("Asked format is not available, the only format currently implemented is CF32."); }

    std::ifstream _file(_path, std::ios::binary);
    if (!_file.is_open()) { throw std::runtime_error("Tried to open file " + _path + ", but it failed to open with error: \"" + std::strerror(errno) + "\"."); }

    return (SoapySDR::Stream *)(this);
}

void SoapyFile::closeStream (SoapySDR::Stream *stream) 
{
    _file.close();
    return;
}

int SoapyFile::activateStream (SoapySDR::Stream *stream, const int flags=0, const long long timeNs=0, const size_t numElems=0) 
{
    _startTime = std::chrono::steady_clock::now();
    _samplesDelivered = 0;
    if (((flags & (1 << 2)) != 0 && timeNs != 0) || numElems != 0) { return SOAPY_SDR_NOT_SUPPORTED; }
    return 0;
}

int SoapyFile::deactivateStream (SoapySDR::Stream *stream, const int flags=0, const long long timeNs=0)
{
    if ((flags & (1 << 2)) != 0 && timeNs != 0) { return SOAPY_SDR_NOT_SUPPORTED; }
    return 0;
}

int SoapyFile::readStream (SoapySDR::Stream *stream, void *const *buffs, const size_t numElems, int &flags, long long &timeNs, const long timeoutUs=100000)
{
    float *out = (float *)buffs[0];
    size_t numberOfComplexSamples = numElems * 2;
    size_t bytesAsked = numberOfComplexSamples * sizeof(float);

    if (_residualLength > 0) 
    {
        std::memcpy(reinterpret_cast<char *>(out), _residual, _residualLength);
    }

    _file.read(reinterpret_cast<char *>(out + _residualLength), bytesAsked - _residualLength);
    std::streamsize bytesRead = _residualLength + _file.gcount();
    if (_file.eof()) 
    {
        /*
         * if (_repeat)
         * {
         *  size_t bytesMissing = bytesAsked - bytesRead;
         *  _file.clear(failbit|eofbit);
         *  _file.setstate(goodbit);
         *  _file.seekg(0);
         *  _file.read(reinterpret_cast<char*>out, bytesMissing)
         * }
         * else
         */
        return SOAPY_SDR_STREAM_ERROR;
    }
    const size_t bytesPerSample = 2 * sizeof(float);
    size_t samplesRead = bytesRead / bytesPerSample;
    size_t bytesRemaining = bytesRead % bytesPerSample;

    _residualLength = bytesRemaining;
    if (bytesRemaining > 0)
    {
        std::memcpy(_residual, reinterpret_cast<char *>(out + samplesRead * bytesPerSample), _residualLength);
    }

    if (samplesRead == 0 && !(_file.eof()))
    {
        return SOAPY_SDR_TIMEOUT;
    }
    
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

void SoapyFile::setSampleRate (const int direction, const size_t channel, const double rate)
{
    _sampleRate = rate;
}

double SoapyFile::getSampleRate (const int direction, const size_t channel) const
{
    return _sampleRate;
}

std::vector<double> SoapyFile::listSampleRates (const int direction, const size_t channel) const
{
    return std::vector<double>{_sampleRate};
}

SoapySDR::RangeList SoapyFile::getSampleRateRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_sampleRate, _sampleRate)};
}

void SoapyFile::setFrequency (const int direction, const size_t channel, const double frequency, const SoapySDR::Kwargs &args=SoapySDR::Kwargs())
{
    _centerFreq = frequency;
}

double SoapyFile::getFrequency (const int direction, const size_t channel) const
{
    return _centerFreq;
}

std::vector<std::string> SoapyFile::listFrequencies (const int direction, const size_t channel) const
{
    return std::vector<std::string>{"RF"};
}

SoapySDR::RangeList SoapyFile::getFrequencyRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_centerFreq - _sampleRate/2, _centerFreq + _sampleRate/2)};
}

std::vector<std::string> SoapyFile::listGains (const int direction, const size_t channel) const
{
    return std::vector<std::string>{};
}

bool SoapyFile::hasGainMode (const int direction, const size_t channel) const
{
    return false;
}

void SoapyFile::setGain (const int direction, const size_t channel, const double value)
{
    return;
}

double SoapyFile::getGain (const int direction, const size_t channel) const
{
    return 0;
}

SoapySDR::Range SoapyFile::getGainRange (const int direction, const size_t channel) const
{
    return SoapySDR::Range(0, 0);
}

std::vector<std::string> SoapyFile::listAntennas (const int direction, const size_t channel) const
{
    return std::vector<std::string>{"RX"};
}

void SoapyFile::setAntenna (const int direction, const size_t channel, const std::string &name)
{
    return;
}

std::string SoapyFile::getAntenna (const int direction, const size_t channel) const
{
    return "RX";
}

void SoapyFile::setBandwidth (const int direction, const size_t channel, const double bw)
{
    _sampleRate = bw;
}

double SoapyFile::getBandwidth (const int direction, const size_t channel) const
{
    return _sampleRate;
}

std::vector<double> SoapyFile::listBandwidths (const int direction, const size_t channel) const
{
    return std::vector<double>{_sampleRate};
}

SoapySDR::RangeList SoapyFile::getBandwidthRange (const int direction, const size_t channel) const
{
    return SoapySDR::RangeList{SoapySDR::Range(_sampleRate, _sampleRate)};
}

/***********************************************************************
 * Find available devices
 **********************************************************************/
static SoapySDR::KwargsList findFile(const SoapySDR::Kwargs &args)
{
    SoapySDR::KwargsList results;

    if  (args.count("driver") > 0 && args.at("driver") == "file")
    {
        SoapySDR::Kwargs device;
        device["label"] = "File Source";
        if (args.count("path") > 0) { device["path"] = args.at("path"); }
        results.push_back(device);
    }

    return results;    
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeFile(const SoapySDR::Kwargs &args)
{
    return new SoapyFile(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerFile("file", &findFile, &makeFile, SOAPY_SDR_ABI_VERSION);
