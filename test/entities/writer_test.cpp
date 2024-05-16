

#include <thread>
#include <chrono>

#include "writer_test.hpp"
#include <w2rp/log.hpp>

using namespace w2rp;

Publisher::Publisher()
{
    logInfo("[APP] App started")
}

Publisher::~Publisher()
{
    
}

bool Publisher::init()
{
    writer = new Writer();
    return true;
}


void Publisher::runThread(uint32_t number_samples)
{    
    logInfo("[APP] Publisher running.")
    for (uint32_t i = 0; i < number_samples; ++i)
    {
        if (!publish())
        {
            --i;
        }
        else
        {
            logInfo("[APP] - Sending sample with index: " << i)
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
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
    Publisher myPub;
    if (myPub.init())
    {
        myPub.run();
    }

    while(true)
    {

    }

    return 0;
}