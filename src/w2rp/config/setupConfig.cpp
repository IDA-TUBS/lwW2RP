#include<w2rp/config/setupConfig.hpp>

using namespace w2rp::config;

setupConfig::setupConfig()
{
}

setupConfig::setupConfig(std::string cfg_path)
:
    config(YAML::LoadFile(cfg_path))
{
}

uint32_t setupConfig::get_hostID(std::string name)
{
    return getAttribute<uint32_t>(name, HOST_ID);
}

void setupConfig::load(std::string cfg_path)
{
    config = YAML::LoadFile(cfg_path);
}