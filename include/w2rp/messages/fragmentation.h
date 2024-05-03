// fragmentation.h
#ifndef FRAGMENTATION_H
#define FRAGMENTATION_H

#include <iostream>
#include <cstring>
#include <vector>

#include "fragment.h"
#include "serializedPayload.h"

class Fragmentation
{
  public:

    Fragment* frag;
    unsigned char* fragData;

    Fragmentation(uint32_t fragmentSize)   
    {
        this->action(fragmentSize);
    };

    ~Fragmentation()  
    {
    };

    void action(uint32_t fragmentSize);

    void fragmentPayload(SerializedPayload* payload, uint32_t fragSize, std::vector<Fragment*>* res, bool compare);
};
#endif // FRAGMENTATION_H