#ifndef WRITERTEST_H
#define WRITERTEST_H

#include <w2rp/entities/writer.hpp>
#include <w2rp/config/writerConfig.hpp>

using namespace w2rp;

class Publisher
{
public:

    Publisher();

    virtual ~Publisher();

    bool init(uint16_t participant_id ,std::string cfg, std::string setup);

    void run();

    bool publish();

private:

    Writer* writer;


    void runThread(uint32_t number_samples);
    int64_t period;
    bool stop_send;
};

#endif //WRITERTEST_H