#ifndef W2RP_READER_CONFIG_h
#define W2RP_READER_CONFIG_h


#include <w2rp/config/config.hpp>
#include <w2rp/config/setupConfig.hpp>
#include <w2rp/comm/socketEndpoint.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp> 
#include <chrono>

namespace w2rp{
namespace config{

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG "w2rp_config.json"
#define DEFAULT_SETUP "setup_defines.json"
#endif


class readerCfg
{
    public:
    /*--------------------- Methods -----------------------*/
    
    /**
     * @brief Construct a new empty reader Cfg object
     * 
     */
    readerCfg();

    /**
     * @brief Construct a new reader Cfg object
     * 
     * @param name reader name
     * @param cfg_path path to config file
     * @param setup_path path to setup file
     */
    readerCfg(
        std::string name, 
        std::string cfg_path = DEFAULT_CONFIG, 
        std::string setup_path = DEFAULT_SETUP
    );

    /**
     * @brief Destroy the reader Cfg object
     * 
     */
    ~readerCfg();

    /**
     * @brief load a configuration
     * 
     * @param name reader name
     * @param cfg_path path to config file
     * @param setup_path path to setup file
     */
    void load(
        std::string name = READER, 
        std::string cfg_path = DEFAULT_CONFIG, 
        std::string setup_path = DEFAULT_SETUP
    );
    
    /**
     * @brief print the configuration 
     * 
     */
    void print();

    /*--------------------- Attribute getter Methods -----------------------*/

    /**
     * @brief get reader endpoint <ip, port>
     * 
     * @return socket_endpoint struct{ip, port}
     */
    w2rp::socket_endpoint endpoint();

    /**
     * @brief get deadline
     * 
     * @return std::chrono::microseconds 
     */
    std::chrono::microseconds deadline();

    /**
     * @brief get response delay
     * 
     * @return std::chrono::microseconds 
     */
    std::chrono::microseconds responseDelay();

    /**
     * @brief get writer endpoint (ip, port)
     * 
     * @return w2rp::socket_endpoint [struct]
     */
    w2rp::socket_endpoint writer();

    /**
     * @brief get cache size
     * 
     * @return unsigned int 
     */
    unsigned int sizeCache();

    /**
     * @brief get host ID
     * 
     * @return uint32_t 
     */
    uint32_t host_id();

    /**
     * @brief get reader priority
     * 
     * @return uint32_t 
     */
    uint32_t priority();

    private:

    bool check();

    template<typename T>
    T getAttribute(std::string name)
    {
        return config.get<T>(id + "." + name);
    }

    std::string id;
    boost::property_tree::ptree config;
    setupConfig setup;
};

}; // end namespace conifg
}; // end namespace w2rp

#endif