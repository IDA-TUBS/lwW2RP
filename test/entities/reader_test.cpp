

#include <thread>
#include <chrono>

#include "reader_test.hpp"
#include <w2rp/log.hpp>
 

using namespace w2rp;

Subscriber::Subscriber()
{
    logInfo("[APP] App started")
}

Subscriber::~Subscriber()
{
    
}
 
bool Subscriber::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    config::readerCfg r_config("READER_01", cfg, setup);

    reader = new Reader(participant_id, r_config);
    return true;
}


void Subscriber::rxThread()
{    
    logInfo("[APP] Subscriber running.")

    SerializedPayload payload;
    while (true)
    {
        reader->retrieveSample(payload);
        logInfo("\n----------------------------------------------------------------------------------------\n[APP] Received sample: " << payload.data << "\n----------------------------------------------------------------------------------------\n");
    }
}



void Subscriber::run()
{    
    std::thread thread(&Subscriber::rxThread, this);
    
    thread.join();
}





int main()
{
    uint16_t p_id = 0x5340;
    std::string cfg_path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + "/lightweightW2RP/examples/setup_defines.json";

    Subscriber mySub;
    if (mySub.init(p_id, cfg_path, setup_path))
    {
        mySub.run();
    }

    while(true)
    {

    }

    return 0;
}