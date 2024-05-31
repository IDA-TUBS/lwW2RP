#ifndef SETUP_CONFIG_h
#define SETUP_CONFIG_h

#include <yaml-cpp/yaml.h>

#include <w2rp/log.hpp>

namespace w2rp{
namespace config{

#ifndef DEFAULT_CONFIG
#define DEFAULT_SETUP "setup_defines.yaml"
#endif


/********************** General ***************************/
#define HOST_ID "HOST_ID"

/**
 * @brief class for storing information about the underlying setup (hosts, etc.)
 * 
 */
class setupConfig
{
    public:

    /**
     * @brief Construct a new empty setup Config object
     * 
     */
    setupConfig();

    /**
     * @brief Construct a new setup Config object
     * 
     * @param cfg_path path to the setup file
     */
    setupConfig(std::string cfg_path);

    /**
     * @brief 
     * 
     * @param cfg_path 
     */
    void load(std::string cfg_path);

    /**
     * @brief Get the hostID object
     * 
     * @param name name of the host
     * @return uint32_t ID of the host
     */
    uint32_t get_hostID(std::string name);

    /**
     * @brief Check if host with specified name exists
     * 
     * @param name host name
     * @return true configuration exists
     * @return false no configuration available
     */
    bool check(std::string name);

    private:

    template<typename T>
    T getAttribute(std::string name, std::string attr)
    {
        if(config[name])
        {
            return config[name][attr].as<T>();
        }
        else
        {
            logError("Host " << name << " does not exist")
            return T();
        }
    }

    YAML::Node config;
};

};  // end namespace config
};   // end namespace w2rp

#endif