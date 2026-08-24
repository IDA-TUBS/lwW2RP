#include <w2rp/log.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

#include "publisher.hpp"
#include "misc.hpp"

/**
 * @brief Basic writer with dummy payload for testing
 *
 * @param argc
 * @param argv [1] log ID suffix [2] sample size [3] = config path [4] max samples
 * @return int
 */
int main(int argc, char* argv[])
{
    // Default configuration
    std::string log_id = "test";
    std::string cfg_path = resolve_config_path(w2rp_config);
    std::string setup_path = resolve_config_path(setup_defines);
    uint64_t sample_size = 80000; // default HD jpeg size
    uint64_t max_samples = 0; // 0 = infinite

    // Check supplied arguments
    if(argc > 1)
    {
        log_id = std::string(argv[1]);
    }
    if(argc > 2)
    {
        sample_size = std::stoull(argv[2]);
    }
    if(argc > 3)
    {
        cfg_path = std::string(argv[3]);
    }
    if(argc > 4)
    {
        max_samples = std::stoull(argv[4]);
    }

    logInfo("W2RP Config: " << cfg_path)

    // Iinitalize logging
    w2rp::init_console_log();
    std::string log_path = get_log_dir();
    std::string log_suffix = "writer_" + generateTimestamp() + "_" + log_id;
    w2rp::init_file_log(log_path, log_suffix);

    uint16_t p_id = 0x8517;

    dummy::Publisher myPub;
    myPub.init(p_id, cfg_path, setup_path, sample_size, max_samples);

    myPub.run();

    return 0;
}
