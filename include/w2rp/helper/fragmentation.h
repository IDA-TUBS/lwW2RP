// fragmentation.h
#ifndef FRAGMENTATION_H
#define FRAGMENTATION_H

#include <iostream>
#include <cstring>
#include <vector>

#include <w2rp/sampleFragment.h>
#include <w2rp/helper/serializedPayload.h>

namespace w2rp {

class SampleFragment;

class Fragmentation
{
  public:

    SampleFragment* frag;
    unsigned char* fragData;

    Fragmentation(uint32_t fragmentSize)   
    {
        this->action(fragmentSize);
    };

    ~Fragmentation()  
    {
    };

    void action(uint32_t fragmentSize);

    void fragmentPayload(SerializedPayload* payload, uint32_t fragSize, std::vector<SampleFragment*>* res, bool compare);
};

}; // end namespace

#endif // FRAGMENTATION_H