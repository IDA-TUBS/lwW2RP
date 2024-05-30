#include <w2rp/guid/guid.hpp>
#include <w2rp/guid/entityID.hpp>
#include <w2rp/guid/guidPrefix.hpp>
#include <w2rp/guid/guidPrefixManager.hpp>

#include <w2rp/config/setupConfig.hpp>

#include <w2rp/log.hpp>

using namespace w2rp;

int main()
{
    std::string path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/setup_defines.yaml";
    
    logInfo("Path: " << path)

    // Load setup config for host ID
    config::setupConfig setup(path);

    uint32_t host_id = setup.get_hostID("TELEMETRY");
    uint16_t participant_id = 0x0101;

    EntityID_t reader(entityID_reader);

    GuidPrefix_t prefix;
    guidPrefixManager::instance().create(host_id, participant_id, prefix);

    GUID_t guid(prefix, reader);
    
    logInfo("GUID: " << guid);

    return 0;
}