#include <w2rp/config/readerConfig.hpp>
#include <w2rp/config/writerConfig.hpp>
#include <w2rp/log.hpp>

using namespace w2rp;

int main()
{
    std::string path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/w2rp_config.yaml";
    
    logInfo("Path: " << path)

    config::readerCfg reader("READER_01", path);
    config::writerCfg writer("WRITER_01", path);

    reader.print();
    writer.print();

    return 0;
}