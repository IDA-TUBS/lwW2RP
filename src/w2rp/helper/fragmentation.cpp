#include <w2rp/helper/fragmentation.hpp>
#include <w2rp/log.hpp>

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
    // logDebug("[Writer - Fragmentation] fragments needed: " << fragmentCount)

    for (uint32_t fragNum = 1; fragNum <= fragmentCount; fragNum++)
    {
        // Calculate fragment start
        uint32_t fragmentStart = fragSize * (fragNum - 1);
        // Calculate fragment size. If last fragment, size may be smaller
        uint32_t fragmentSize = fragNum < fragmentCount ? fragSize : payload->length - fragmentStart;
        unsigned char fragData[this->fragSize]; // TODO using fragmentSize does not work?!
        memset(fragData, 0, this->fragSize * sizeof(unsigned char));
        memcpy(fragData, &(payload->data[fragmentStart]), fragmentSize);

        // logDebug("[Writer - Fragmentation] fragment " << fragNum << " size: " << fragmentSize << " data: " << fragData)

        SampleFragment *fragment = new SampleFragment(baseChange, fragNum - 1, fragmentSize, arrivalTime);

        fragment->setData(fragData, fragmentSize, fragNum - 1, arrivalTime);
        fragment->setBaseChange(baseChange);
        
        // if(compare)
        // {
        //     bool same = (*fragment == *(res->at(fragNum-1)));
        // }

        res->push_back(fragment);
        
    } 
}

}; // end namespace