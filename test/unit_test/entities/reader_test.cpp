

#include <thread>
#include <chrono>

#include "../common.hpp"

#include "reader_test.hpp"
#include <w2rp/log.hpp>
 

using namespace w2rp;

int main()
{
    uint16_t p_id = 0x5340;

    std::string cfg_path = resolve_config_path(w2rp_config);
    std::string setup_path = resolve_config_path(setup_defines);

    logInfo("Using config path: " << cfg_path)
    logInfo("Using setup path: " << setup_path)

    config::readerCfg r_config("READER_01", cfg_path, setup_path);
    Subscriber mySub(p_id, r_config);

    mySub.run();

    while(true)
    {

    }

    return 0;
}