#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <atomic>

#include <w2rp/config/readerConfig.hpp>
#include <w2rp/helper/safe_queue.hpp>
#include <w2rp/entities/reader.hpp>

#include "misc.hpp"

using namespace w2rp;

namespace dummy {

class Subscriber
{
public:

    Subscriber():
        receiver{}
    {
        logInfo("[APP] App started")
    }

    ~Subscriber()
    {}

    void init(uint16_t participant_id, std::string cfg, std::string setup, uint64_t sample_size, uint64_t max_sample=0)
    {
        max_sample_num = max_sample;
        sample_size_ = sample_size;
        logInfo("[APP] Subscriber max samples: " << max_sample_num)
        logInfo("[APP] Subscriber sample size: " << sample_size_)

        w2rp::config::readerCfg r_config("READER_01", cfg, setup);
        reader = new w2rp::Reader(participant_id, r_config);

        logInfo("[APP] Subscriber max samples: " << max_sample_num)
        logInfo("[APP] Subscriber sample size: " << sample_size_)
    }

    void run()
    {    
        receiver = std::thread(&Subscriber::receive_sample, this);
        join();
        logInfo("[APP] Subscriber finished.")
    }

private:

    w2rp::Reader* reader;

    std::thread receiver;

    uint64_t sample_size_;
    uint64_t max_sample_num; 

    void receive_sample()
    {    
        logInfo("[APP] Subscriber running: " << max_sample_num << " samples")

        w2rp::Reader::sample sample;
        uint64_t sequence_num = 0;

        while(true)
        {
            reader->retrieveSample(sample);
            logInfo("[APP] Received sample: " << sample.first);

            sequence_num = sample.first;

            if((max_sample_num != 0) && (sequence_num >= (max_sample_num-1)))
            {
                logInfo("[APP] Last sample received: " << sequence_num << "/" << (max_sample_num-1));
                break;
            }
        }
        // delay program end to ensure successfull transmission of last sample
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    void join()
    {
        receiver.join();
    }


};

} // namespace dummy

#endif //SUBSCRIBER_H
