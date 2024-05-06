// file.cpp
#include <w2rp/helper/fragmentation.h>

namespace w2rp {

Fragmentation::Fragmentation(uint32_t fragmentSize)   
{
    this->fragSize = fragmentSize;
    
};

Fragmentation::~Fragmentation()
{
    //TODO
}

void Fragmentation::fragmentPayload(SerializedPayload* payload, CacheChange *baseChange, std::vector<SampleFragment*>* res, std::chrono::system_clock::time_point arrivalTime, bool compare)
{
    uint32_t fragmentCount = (payload->length + this->fragSize - 1) / this->fragSize; 

    for (uint32_t fragNum = 1; fragNum <= fragmentCount; fragNum++)
    {
        // Calculate fragment start
        uint32_t fragmentStart = fragSize * (fragNum - 1);
        // Calculate fragment size. If last fragment, size may be smaller
        uint32_t fragmentSize = fragNum < fragmentCount ? fragSize : payload->length - fragmentStart;

        unsigned char *fragData = new unsigned char[this->fragSize]; 
        std::memcpy(fragData, &(payload->data[fragmentStart]), fragmentSize);

        SampleFragment *fragment = new SampleFragment();

        fragment->setData(fragData, fragmentSize, fragNum - 1, arrivalTime);
        fragment->setBaseChange(baseChange);
        
        if(compare)
        {
            bool same = (*fragment == *(res->at(fragNum-1)));
        }

        res->push_back(fragment);
    } 
}

}; // end namespace