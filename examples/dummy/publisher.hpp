#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <w2rp/entities/writer.hpp>
#include <w2rp/config/writerConfig.hpp>

namespace dummy {

class Publisher
{
    public:

        Publisher()
        {}

        virtual ~Publisher()
        {}

        void init(uint16_t participant_id, std::string cfg, std::string setup, uint64_t sample_size, int max_samples=0)
        {
            w2rp::config::writerCfg w_config("WRITER_01", cfg, setup);
            writer = new w2rp::Writer(participant_id, w_config);

            payload = w2rp::SerializedPayload(sample_size, 0);

            sample_size_ = sample_size;
            max_sample_num = max_samples;
            logInfo("[APP] Publisher max samples: " << max_sample_num)
            logInfo("[APP] Publisher sample size: " << sample_size_)
        }

        void run()
        {    
            std::thread thread(&Publisher::runThread, this);
            
            thread.join();
            logInfo("\n[APP] Publisher finished.")
        }

        bool publish()
        {
            writer->write(payload);
        }

    private:

        w2rp::Writer* writer;
        w2rp::SerializedPayload payload;

        uint64_t sample_size_;
        int max_sample_num;

        void runThread()
        {    
            logInfo("\n[APP] Publisher running.")
            uint32_t i = 0;
            while(true)
            {
                publish();
                i++;

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                if((max_sample_num != 0) && (i >= max_sample_num))
                {
                    logInfo("[APP] transmitted last sample: " << i << "/" << max_sample_num)
                    break;
                }
                logInfo("[APP] iterator: " << i << "/" << max_sample_num)
            }
            
            // delay program end to ensure successfull transmission of last sample
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
};

} // namespace dummy

#endif //PUBLISHER_H
