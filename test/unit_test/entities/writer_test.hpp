#ifndef WRITERTEST_H
#define WRITERTEST_H

#include <w2rp/entities/writer.hpp>
#include <w2rp/config/writerConfig.hpp>

using namespace w2rp;

class Publisher
{
public:

    Publisher(
        std::uint16_t participant_id,
        config::writerCfg cfg
    ):
        writer(participant_id, cfg)
    {

    };

    virtual ~Publisher(){};

    void run()
    {    
        int number_samples = 100;
        std::thread thread(&Publisher::runThread, this, number_samples);
        
        thread.join();
    }

    void publish()
    {
        unsigned char data[] = {'W', '2', 'R', 'P', ' ', 'T', 'E', 'S', 'T', ' ', 'M', 'E', 'S', 'S', 'A', 'G', 'E'};
        uint32_t dataSize = sizeof(data) / sizeof(data[0]);
    
        w2rp::SerializedPayload payload(data, data + dataSize);

        writer.write(payload);
    }

private:

    Writer writer;

    int64_t period;
    bool stop_send;

    void runThread(uint32_t number_samples)
    {    
        logInfo("\n[APP] Publisher running.")
        for (uint32_t i = 0; i < number_samples; ++i)
        {
            logInfo("\n----------------------------------------------------------------------------------------\n[APP] - Sending sample with index: " << i << "\n----------------------------------------------------------------------------------------\n")
            publish();
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
        
    }


};

#endif //WRITERTEST_H