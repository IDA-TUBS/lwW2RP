#ifndef READERTEST_H
#define READERTEST_H

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

#endif //READERTEST_H