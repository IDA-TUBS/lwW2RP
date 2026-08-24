#include <thread>
#include <chrono>

#include "../common.hpp"

#include "writer_test.hpp"
#include <w2rp/log.hpp>

using namespace w2rp;

int main()
{
    std::uint16_t p_id = 0x8517;
    std::string cfg_path = resolve_config_path(w2rp_config);
    std::string setup_path = resolve_config_path(setup_defines);

    logInfo("Using config path: " << cfg_path)
    logInfo("Using setup path: " << setup_path)

    config::writerCfg w_config("WRITER_01", cfg_path, setup_path);

    Publisher myPub(p_id, w_config);

    myPub.run();

    while(true)
    {

    }

    return 0;
}