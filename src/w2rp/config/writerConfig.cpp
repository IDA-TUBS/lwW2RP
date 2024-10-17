#include <w2rp/config/writerConfig.hpp>
#include <stdexcept>

using namespace w2rp::config;

writerCfg::writerCfg()
{

}

writerCfg::writerCfg(std::string name, std::string cfg_path, std::string setup_path)
{
    load(name, cfg_path, setup_path);
}

writerCfg::~writerCfg()
{

}

void writerCfg::load(std::string name, std::string cfg_path, std::string setup_path)
{
    id = name;    
    boost::property_tree::read_json(cfg_path, config);     
    setup.load(setup_path);
    check();
}

void writerCfg::print()
{
    logInfo("#---------- Writer Configuration -------------#")
    logInfo("# Address: " << endpoint().ip_addr)
    logInfo("# Port: " << endpoint().port)
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

w2rp::socket_endpoint writerCfg::endpoint()
{
    std::string address = getAttribute<std::string>(ADDRESS);
    int port = getAttribute<int>(PORT);
    return socket_endpoint(address, port);
}

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

std::chrono::microseconds writerCfg::timeout()
{
    // yaml-cpp does not support direct conversion to std::chrono(!)
    int duration = getAttribute<int>(TIMEOUT);
    return std::chrono::microseconds(duration);
}

size_t writerCfg::numberReaders()
{
    return config.get_child(id + "." + READERS).size();
}

w2rp::socket_endpoint writerCfg::readerEndpoint(int index)
{
    // get readers child node
    auto readers = config.get_child(id + "." + READERS);

    // Access by index
    auto reader_it = readers.begin();
    std::advance(reader_it, index);

    // get reader name
    std::string name = reader_it->second.get_value<std::string>();
    
    // Access reader config
    std::string address = config.get<std::string>(name + "." + ADDRESS);
    int port = config.get<int>(name + "." + PORT);
    
    return w2rp::socket_endpoint(address, port);
}

std::vector<w2rp::socket_endpoint> writerCfg::readerEndpoints()
{
    std::vector<w2rp::socket_endpoint> readers;

    // get readers child node
    auto reader_names = config.get_child(id + "." + READERS);
    for(auto it = reader_names.begin(); it != reader_names.end(); it++)
    {
        std::string reader = it->second.get_value<std::string>();
        std::string address = config.get<std::string>(reader + "." + ADDRESS);
        int port = config.get<int>(reader + "." + PORT);
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

unsigned int writerCfg::aggregation_size()
{
    return getAttribute<unsigned int>(AGGREGATION_SIZE);
}


std::vector<uint32_t> writerCfg::reader_id()
{
    std::vector<uint32_t> readerList;

    // get readers child node
    auto reader_names = config.get_child(id + "." + READERS);
    for(auto it = reader_names.begin(); it != reader_names.end(); it++)
    {
        std::string reader = it->second.get_value<std::string>();
        std::string host = config.get<std::string>(reader + "." + HOST);
        uint32_t id = setup.get_hostID(host);
        readerList.push_back(id);
    }   
    return readerList;
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
    if(config.find(id) == config.not_found())
    {
        logError("No configuration found for " << id);
        throw std::invalid_argument(id + " not found");
        return false;
    }

    // Check Reader IDs
    auto reader_names = config.get_child(id + "." + READERS);
    for(auto it = reader_names.begin(); it != reader_names.end(); it++)
    {
        std::string reader = it->second.get_value<std::string>();
        if(config.find(reader) == config.not_found())
        {
            logError("No configuration found for assigned reader " << reader)
            throw std::invalid_argument(reader + " not found");
            return  false;
        }
    }

    // Check Host
    std::string hostName = getAttribute<std::string>(HOST);
    if(!setup.check(hostName))
    {
        logError("No configuration for assigned host " << hostName);
        throw std::invalid_argument(hostName + " not found");
        return false;
    }

    // print attributes to validate configuration parameters (availability+type)
    print();

    return true;
}