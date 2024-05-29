

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

bool Subscriber::init()
{
    reader = new Reader();
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
    Subscriber mySub;
    if (mySub.init())
    {
        mySub.run();
    }

    while(true)
    {

    }

    return 0;
}