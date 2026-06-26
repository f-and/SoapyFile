#ifndef SOAPYFILE_HPP
#define SOAPYFILE_HPP

#include <cstring>
#include <stdexcept>
#include <cerrno>
#include <sys/stat.h>
#include <fstream>
#include <chrono>
#include <thread>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Constants.h>

/***********************************************************************
 * Device interface
 **********************************************************************/
class SoapyFile : public SoapySDR::Device
{
    public:
        SoapyFile(const SoapySDR::Kwargs &args);

        std::string getDriverKey(void) const override;
        std::string getHardwareKey(void) const override;
        std::vector<std::string> getStreamFormats (const int direction, const size_t channel) const override; 
        std::string getNativeStreamFormat (const int direction, const size_t channel, double &fullScale) const override;
        size_t getNumChannels (const int direction) const override;
        SoapySDR::Stream* setupStream (const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args) override;
        void closeStream (SoapySDR::Stream *stream) override;
        int activateStream (SoapySDR::Stream *stream, const int flags, const long long timeNs, const size_t numElems) override;
        int deactivateStream (SoapySDR::Stream *stream, const int flags, const long long timeNs) override;
        int readStream (SoapySDR::Stream *stream, void *const *buffs, const size_t numElems, int &flags, long long &timeNs, const long timeoutUs) override;

    private: 
        std::string _path;
        double _sampleRate;
        double _centerFreq;
        bool _repeat;        
        bool _isFifo;        
        std::ifstream _file;
        std::chrono::steady_clock::time_point _startTime;
        long long _samplesDelivered;
        char _residual[8];
        size_t _residualLength;

    //Implement constructor with device specific arguments...

    //Implement all applicable virtual methods from SoapySDR::Device
};

#endif
