#include <w2rp/config/readerConfig.hpp>
#include <w2rp/config/writerConfig.hpp>
#include <w2rp/log.hpp>

using namespace w2rp;

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        logError("Usage: readConfig_test <w2rp_source_path>")
        return -1;
    }
    else
    {
        logInfo("W2RP source path: " << argv[1])
    }
    std::string w2rp_source_path = argv[1];

    std::string cfg_path = std::string(getenv("HOME")) + w2rp_source_path + "/test/unit_test/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + w2rp_source_path + "/test/unit_test/setup_defines.json";

    logInfo("CFG Path: " << cfg_path)
    logInfo("Setup Path: " << setup_path)

    config::readerCfg reader("READER_01", cfg_path, setup_path);
    config::writerCfg writer("WRITER_01", cfg_path, setup_path);

    return 0;
}