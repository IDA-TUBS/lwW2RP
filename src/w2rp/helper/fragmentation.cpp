// file.cpp
#include <w2rp/helper/fragmentation.h>


void Fragmentation::action(uint32_t fragmentSize)
{
    std::cout << "Fragmenting large samples!" << std::endl;
    frag = new Fragment();
    fragData = new unsigned char[fragmentSize];
}

void Fragmentation::fragmentPayload(SerializedPayload* payload, uint32_t fragSize, std::vector<Fragment*>* res, bool compare)
{
    uint32_t fragmentCount = (payload->length + fragSize - 1) / fragSize;    

    for (uint32_t fragNum = 1; fragNum <= fragmentCount; fragNum++)
    {
        // Calculate fragment start
        uint32_t fragmentStart = fragSize * (fragNum - 1);
        // Calculate fragment size. If last fragment, size may be smaller
        uint32_t fragmentSize = fragNum < fragmentCount ? fragSize : payload->length - fragmentStart;

        std::memcpy(fragData, &(payload->data[fragmentStart]), fragmentSize);

        frag->setData(fragData, fragmentSize);

        if(compare)
        {
            bool same = (*frag == *(res->at(fragNum-1)));
        }

        res->at(fragNum-1)->setData(fragData, fragmentSize);
        // delete frag;
        // TODO: delete frags somewhere
    } 
}