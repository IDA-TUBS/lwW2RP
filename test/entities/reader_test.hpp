#ifndef WRITERTEST_H
#define WRITERTEST_H

#include <w2rp/entities/reader.hpp>

using namespace w2rp;

class Subscriber
{
public:

    Subscriber();

    virtual ~Subscriber();

    bool init();

    void run();

    bool publish();

private:

    Reader* reader;

    void rxThread();
};

#endif //WRITERTEST_H