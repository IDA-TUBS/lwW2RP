#ifndef READERTEST_H
#define READERTEST_H

#include <w2rp/entities/reader.hpp>
#include <w2rp/config/readerConfig.hpp>

using namespace w2rp;

class Subscriber
{
public:

    Subscriber(
        std::uint16_t participant_id,
        config::readerCfg cfg
    ):
        reader(participant_id, cfg)
    {
        logInfo("[APP] App started")
    };

    virtual ~Subscriber(){}

    void run()
    {    
        std::thread thread(&Subscriber::rxThread, this);
        thread.join();
    }

    bool publish();

private:

    Reader reader;

    void rxThread()
    {    
        logInfo("[APP] Subscriber running.")

        w2rp::Reader::sample sample;
        while (true)
        {
            reader.retrieveSample(sample);
            logInfo("\n----------------------------------------------------------------------------------------\n[APP] Received sample: " << sample.first << "\n---------------------------------------------------------\n")
            std::string msg(sample.second.begin(), sample.second.end());
            logInfo("[APP] Sample data: " << msg)
        }
    }
};

#endif //READERTEST_H