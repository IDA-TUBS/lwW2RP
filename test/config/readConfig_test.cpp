#include <w2rp/config/readerConfig.hpp>
#include <w2rp/config/writerConfig.hpp>
#include <w2rp/log.hpp>

using namespace w2rp;

int main()
{
    std::string cfg_path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/w2rp_config.yaml";
    std::string setup_path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/w2rp_config.yaml";

    logInfo("CFG Path: " << cfg_path)
    logInfo("Setup Path: " << setup_path)

    // config::readerCfg reader("READER_01", cfg_path, setup_path);
    config::writerCfg writer("WRITER_01", cfg_path, setup_path);

    // reader.print();
    writer.print();

    return 0;
}