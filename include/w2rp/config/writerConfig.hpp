#ifndef W2RP_WRITER_CONFIG_h
#define W2RP_WRITER_CONFIG_h

#include <w2rp/entities/writer.hpp>
#include <w2rp/config/config.hpp>
#include <w2rp/config/setupConfig.hpp>
#include <w2rp/comm/socketEndpoint.hpp>
#include <w2rp/log.hpp>

#include <yaml-cpp/yaml.h>
#include <chrono>

namespace w2rp{
namespace config{

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG "w2rp_config.yaml"
#define DEFAULT_SETUP "setup_defines.yaml"
#endif

class writerCfg
{
    public:
    /*--------------------- Constructor/Destructor -----------------------*/
    writerCfg();

    writerCfg(
        std::string name = WRITER, 
        std::string cfg_path = DEFAULT_CONFIG, 
        std::string setup_path = DEFAULT_SETUP
    );

    ~writerCfg();
    
    void load(
        std::string name = WRITER, 
        std::string cfg_path = DEFAULT_CONFIG,
        std::string setup_path = DEFAULT_SETUP
    );

    void print();

    /*--------------------- Attribute getter Methods -----------------------*/
    
    /**
     * @brief get fragment size
     * 
     * @return uint32_t 
     */
    uint32_t fragmentSize();
    
    /**
     * @brief get deadline
     * 
     * @return std::chrono::microseconds 
     */
    std::chrono::microseconds deadline();
    
    /**
     * @brief get shaping time
     * 
     * @return std::chrono::microseconds 
     */
    std::chrono::microseconds shapingTime();
    
    /**
     * @brief get nack suppression duration
     * 
     * @return std::chrono::microseconds 
     */
    std::chrono::microseconds nackSuppressionDuration();
    
    /**
     * @brief get number of readers
     * 
     * @return size_t 
     */
    size_t numberReaders();
    
    /**
     * @brief get specific reader address
     * 
     * @param index reader index
     * @return w2rp::socket_endpoint 
     */
    socket_endpoint readerEndpoint(int index);
    
    /**
     * @brief get all reader addresses
     * 
     * @return std::vector<w2rp::socket_endpoint> 
     */
    std::vector<socket_endpoint> readerEndpoints();
    
    /**
     * @brief get cache size
     * 
     * @return unsigned int 
     */
    unsigned int sizeCache();
    
    /**
     * @brief get writer UUID
     * 
     * @return uint8_t 
     */
    uint8_t uuid();
    
    /**
     * @brief get prioritization mode
     * 
     * @return PrioritizationMode 
     */
    PrioritizationMode prioMode();

    private:

    template<typename T>
    T getAttribute(std::string name)
    {
        return config[id][name].as<T>();
    }
    
    std::string id;
    YAML::Node config;
    setupConfig setup;
};

}; // end namespace conifg
}; // end namespace w2rp

#endif