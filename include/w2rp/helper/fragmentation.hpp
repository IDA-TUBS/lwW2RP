// fragmentation.h
#ifndef FRAGMENTATION_H
#define FRAGMENTATION_H

#include <iostream>
#include <cstring>
#include <vector>

#include <w2rp/sampleFragment.hpp>
#include <w2rp/helper/serializedPayload.hpp>

namespace w2rp {

class SampleFragment;

class Fragmentation
{
  public:
    uint32_t fragSize;

    Fragmentation(uint32_t fragmentSize);

    ~Fragmentation();

    void fragmentPayload(SerializedPayload* payload, CacheChange *baseChange, std::vector<SampleFragment*>* res, std::chrono::system_clock::time_point arrivalTime, bool compare);
};

}; // end namespace

#endif // FRAGMENTATION_H