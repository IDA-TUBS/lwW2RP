#include <w2rp/config/readerConfig.hpp>
#include <stdexcept>

using namespace w2rp::config;

readerCfg::readerCfg()
{

};

readerCfg::readerCfg(std::string name, std::string cfg_path, std::string setup_path)
:
    id(name),
    config(YAML::LoadFile(cfg_path)),
    setup(setup_path)
{
    check();
};

readerCfg::~readerCfg()
{

};

void readerCfg::load(std::string name, std::string cfg_path, std::string setup_path)
{
    id = name;
    config = YAML::LoadFile(cfg_path);
    setup.load(setup_path);
    check();
}

void readerCfg::print()
{
    logInfo("#---------- Reader Configuration -------------#")
    logInfo("# deadline: " << deadline().count() << "us")
    logInfo("# response delay: " << responseDelay().count() << "us")
    logInfo("# cache size: " << unsigned(sizeCache()))
    logInfo("# host ID: " << unsigned(host_id()))
    logInfo("# priority: " << unsigned(priority()))
    logInfo("#---------------------------------------------#")
}

/*----------------------------------- Attribute getter Methods --------------------------------------*/

w2rp::socket_endpoint readerCfg::endpoint()
{
    std::string address = getAttribute<std::string>(ADDRESS);
    int port = getAttribute<int>(PORT);
    return socket_endpoint(address, port);
}

std::chrono::microseconds readerCfg::deadline()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(DEADLINE);
    return std::chrono::microseconds(duration);
}

std::chrono::microseconds readerCfg::responseDelay()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(RESPONSE_DELAY);
    return std::chrono::microseconds(duration);
}

w2rp::socket_endpoint readerCfg::writer()
{
    std::string name = getAttribute<std::string>(WRITER);

    std::string address = config[name][ADDRESS].as<std::string>();
    int port = config[name][PORT].as<int>();
    return w2rp::socket_endpoint(address, port);
}

unsigned int readerCfg::sizeCache()
{
    return getAttribute<unsigned int>(SIZE_CACHE);
}

uint32_t readerCfg::host_id()
{
    std::string host = getAttribute<std::string>(HOST);
    return setup.get_hostID(host);
}

uint32_t readerCfg::priority()
{
    return getAttribute<uint32_t>(PRIORITY);
}

/*------------------------------------- Private -----------------------------------------*/
bool readerCfg::check()
{
    // Check Reader ID
    if(!config[id])
    {
        logError("No configuration found for " << id)
        throw std::invalid_argument(id + " not found");
        return false;
    }

    // Check Writer ID
    std::string name = getAttribute<std::string>(WRITER);
    if(!config[name])
    {
        logError("No configuration for assigned writer " << name)
        throw std::invalid_argument(name + " not found");
        return false;
    }

    // Check Host
    name = getAttribute<std::string>(HOST);
    if(!setup.check(name))
    {
        logError("No configuration for assigned host " << name)
        throw std::invalid_argument(name + " not found");
        return false;
    }

    // print attributes to validate configuration parameters (availability+type)
    print();

    return true;
}