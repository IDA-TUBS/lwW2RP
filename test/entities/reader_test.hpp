#ifndef WRITERTEST_H
#define WRITERTEST_H

#include <w2rp/entities/reader.hpp>
#include <w2rp/config/readerConfig.hpp>

using namespace w2rp;

class Subscriber
{
public:

    Subscriber();

    virtual ~Subscriber();

    bool init(uint16_t participant_id, std::string cfg, std::string setup);

    void run();

    bool publish();

private:

    Reader* reader;

    void rxThread();
};

#endif //WRITERTEST_H