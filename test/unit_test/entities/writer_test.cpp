

#include <thread>
#include <chrono>

#include "writer_test.hpp"
#include <w2rp/log.hpp>

using namespace w2rp;

Publisher::Publisher()
{

}

Publisher::~Publisher()
{
    
}

bool Publisher::init(uint16_t participant_id ,std::string cfg, std::string setup)
{
    config::writerCfg w_config("WRITER_01", cfg, setup);
    
    writer = new Writer(participant_id, w_config);
    // writer = new Writer(participant_id);
    // writer = Writer(participant_id);
    return true;
}


void Publisher::runThread(uint32_t number_samples)
{    
    logInfo("\n[APP] Publisher running.")
    for (uint32_t i = 0; i < number_samples; ++i)
    {
        logInfo("\n----------------------------------------------------------------------------------------\n[APP] - Sending sample with index: " << i << "\n----------------------------------------------------------------------------------------\n")
        if (!publish())
        {
            --i;
        }
        else
        {
            // sending worked?!
        }
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }
    
}


void Publisher::run()
{    
    int number_samples = 100;
    std::thread thread(&Publisher::runThread, this, number_samples);
    
    thread.join();
}


bool Publisher::publish()
{
    unsigned char data[] = {'W', '2', 'R', 'P', ' ', 'T', 'E', 'S', 'T', ' ', 'M', 'E', 'S', 'S', 'A', 'G', 'E'};
    uint32_t dataSize = sizeof(data) / sizeof(data[0]);
    
    SerializedPayload *payload = new SerializedPayload(data, dataSize);
    // if(writer->write(payload))
    if(writer->write(payload))
    {
        // sample transmitted successfully
        return true;
    }
    else
    {
        // problem occured
        return false;
    }
    
}


int main()
{
    uint16_t p_id = 0x8517;
    std::string cfg_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/examples/w2rp_config.json";
    std::string setup_path = std::string(getenv("HOME")) + "/Documents/Code/lightweightW2RP/examples/setup_defines.json";
    
    Publisher myPub;
    if (myPub.init(p_id, cfg_path, setup_path))
    {
        myPub.run();
    }

    while(true)
    {

    }

    return 0;
}