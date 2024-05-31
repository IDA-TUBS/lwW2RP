#include <w2rp/config/writerConfig.hpp>
#include <stdexcept>

using namespace w2rp::config;

writerCfg::writerCfg()
{

};

writerCfg::writerCfg(std::string name, std::string cfg_path, std::string setup_path)
:
    id(name),
    config(YAML::LoadFile(cfg_path)),
    setup(setup_path)
{
    check();
};

writerCfg::~writerCfg()
{

};

void writerCfg::load(std::string name, std::string cfg_path, std::string setup_path)
{
    id = name;
    config = YAML::LoadFile(cfg_path);
    setup.load(setup_path);
    check();
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
    logInfo("# host ID: " << unsigned(host_id()))
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
    return getAttribute<std::vector<std::string>>(READERS).size();
}

w2rp::socket_endpoint writerCfg::readerEndpoint(int index)
{
    std::string name = getAttribute<std::vector<std::string>>(READERS).at(index);

    std::string address = config[name][ADDRESS].as<std::string>();
    int port = config[name][PORT].as<int>();

    return socket_endpoint(address, port);
}

std::vector<w2rp::socket_endpoint> writerCfg::readerEndpoints()
{
    std::vector<socket_endpoint> readers;

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
    return getAttribute<unsigned int>(SIZE_CACHE);
}

uint32_t writerCfg::host_id()
{
    std::string host = getAttribute<std::string>(HOST);
    return setup.get_hostID(host);
}

w2rp::PrioritizationMode writerCfg::prioMode()
{
    // yaml-cpp does not support direct conversion to custom types by default
    int prioMode = getAttribute<int>(PRIO_MODE);
    return static_cast<w2rp::PrioritizationMode>(prioMode);
}

/*------------------------------------- Private -----------------------------------------*/
bool writerCfg::check()
{
    // Check Writer ID
    if(!config[id])
    {
        logError("No configuration found for " << id)
        throw std::invalid_argument(id + " not found");
        return false;
    }

    // Check Reader IDs
    std::vector<std::string> reader_names = getAttribute<std::vector<std::string>>(READERS);
    for(auto it = reader_names.begin(); it != reader_names.end(); it++)
    {
        if(!config[*it])
        {
            logError("No configuration found for assigned reader " << *it)
            throw std::invalid_argument(*it + " not found");
            return  false;
        }
    }
    
    // Check Host
    std::string name = getAttribute<std::string>(HOST);
    if(!setup.check(name))
    {
        logError("No configuration for assigned host " << name)
        throw std::invalid_argument(name + " not found");
        return false;
    }

    return true;
}