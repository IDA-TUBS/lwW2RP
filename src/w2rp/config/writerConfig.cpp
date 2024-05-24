#include <w2rp/config/writerConfig.hpp>

using namespace w2rp::config;

writerCfg::writerCfg()
{

};

writerCfg::writerCfg(std::string name, std::string cfg_path)
:
    id(name),
    config(YAML::LoadFile(cfg_path))
{

};

writerCfg::~writerCfg()
{

};

void writerCfg::load(std::string name, std::string cfg_path)
{
    id = name;
    config = YAML::LoadFile(cfg_path);
}

void writerCfg::print()
{
    logInfo("#---------- Writer Configuration -------------#")
    logInfo("# fragment size: " << fragmentSize())
    logInfo("# deadline: " << deadline().count() << "us")
    logInfo("# shaping time: " << shapingTime().count() << "us")
    logInfo("# nack suppression duration: " << nackSuppressionDuration().count() << "us")
    logInfo("# number of readers: " << numberReaders())
    logInfo("# cache size: " << unsigned(sizeCache()))
    logInfo("# UUID: " << unsigned(uuid()))
    logInfo("# prioritization mode: " << prioMode())
    logInfo("#---------------------------------------------#")
}

/*----------------------------------- Attribute getter Methods --------------------------------------*/

uint32_t writerCfg::fragmentSize()
{
    return getAttribute<uint32_t>(FRAGMENT_SIZE);
}

std::chrono::microseconds writerCfg::deadline()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(DEADLINE);
    return std::chrono::microseconds(duration);
}

std::chrono::microseconds writerCfg::shapingTime()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(SHAPING_TIME);
    return std::chrono::microseconds(duration);
}

std::chrono::microseconds writerCfg::nackSuppressionDuration()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(NACK_SUPPRESSION_DURATION);
    return std::chrono::microseconds(duration);
}

size_t writerCfg::numberReaders()
{
    // return (config[WRITER][READERS].as<std::vector<std::string>>()).size();
    return getAttribute<std::vector<std::string>>(READERS).size();
}

w2rp::socket_endpoint writerCfg::readerEndpoint(int index)
{
    // std::string name = config[WRITER][READERS].as<std::vector<std::string>>().at(index);

    std::string name = getAttribute<std::vector<std::string>>(READERS).at(index);

    std::string address = config[name][ADDRESS].as<std::string>();
    int port = config[name][PORT].as<int>();

    return socket_endpoint(address, port);
}

std::vector<w2rp::socket_endpoint> writerCfg::readerEndpoints()
{
    std::vector<socket_endpoint> readers;

    // std::vector<std::string> reader_names = config[WRITER][READERS].as<std::vector<std::string>>();
    std::vector<std::string> reader_names = getAttribute<std::vector<std::string>>(READERS);

    for(auto it = reader_names.begin(); it != reader_names.end(); it++)
    {
        std::string address = config[*it][ADDRESS].as<std::string>();
        int port = config[*it][PORT].as<int>();
        readers.push_back(socket_endpoint(address, port));
    }

    return readers;
}

unsigned int writerCfg::sizeCache()
{
    // return config[WRITER][SIZE_CACHE].as<unsigned int>();
    return getAttribute<unsigned int>(SIZE_CACHE);
}

uint8_t writerCfg::uuid()
{
    return getAttribute<uint8_t>(UUID);
}

w2rp::PrioritizationMode writerCfg::prioMode()
{
    // yaml-cpp does not support direct conversion to custom types by default
    int prioMode = getAttribute<int>(PRIO_MODE);
    return static_cast<w2rp::PrioritizationMode>(prioMode);
}