#ifndef W2RP_CONFIG_h
#define W2RP_CONFIG_h

#include <w2rp/entities/writer.hpp>
#include <w2rp/entities/reader.hpp>

#include <yaml-cpp/yaml.h>
#include <chrono>

namespace w2rp{

class writerCfg_
{
    public:
    /*---------------------- Attributes -----------------------*/
    uint32_t fragmentSize;
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration shapingTime;
    std::chrono::system_clock::duration nackSuppressionDuration;
    uint8_t numberReaders;
    std::vector<std::string> readerAddresses;
    unsigned int sizeCache;
    uint8_t writerUuid;
    unsigned char guidPrefix[12];
    PrioritizationMode prioMode;

    /*--------------------- Methods -----------------------*/
    writerCfg_();

    ~writerCfg_();

    private:

};

class readerCfg_
{
    public:
    /*--------------------- Attributes -----------------------*/
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration responseDelay;
    std::string writerAddresses;
    unsigned int sizeCache;
    uint8_t readerUuid;
    unsigned char guidPrefix[12];
    uint32_t priority;

    /*--------------------- Methods -----------------------*/
    readerCfg_();

    ~readerCfg_();

    private:
};





};

#endif