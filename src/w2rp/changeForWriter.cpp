/*
 *
 */

#include <w2rp/changeForWriter.hpp>
#include <w2rp/sampleFragment.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

uint32_t ChangeForWriter::receivedCount()
{
    // count received fragments
    uint32_t received = 0;
    for(uint32_t i = 0; i < numberFragments; i++){
        auto frag = sampleFragmentArray[i];
        if(frag->received) {
            received++;
        }
    }
    return received;
};

bool ChangeForWriter::setFragmentStatus(fragmentStates status, uint32_t fragmentNumber)
{
    auto frag = sampleFragmentArray[fragmentNumber];

    switch(status)
    {
        case RECEIVED:
            lastReceivedFN = fragmentNumber;
            frag->received = true;
            if(fragmentNumber > highestFNreceived)
            {
                highestFNreceived = fragmentNumber;
            }

            break;
        default:
            break;
    }

    return true;
}


bool ChangeForWriter::setFragmentData(uint32_t fragmentNumber, unsigned char *data, uint32_t dataLength)
{
    auto frag = sampleFragmentArray[fragmentNumber];

    if(frag != nullptr)
    {
        auto t = std::chrono::system_clock::now();
        frag->setData(data, dataLength, fragmentNumber, t);
        // logDebug("[ChangeForWriter] update fragment " << fragmentNumber << ": " << frag->data)
    }

    return true;    
}


void ChangeForWriter::getMissingFragments(uint32_t lastfragmentNum, std::vector<uint32_t> *res)
{
    // logDebug("[ChangeForWriter] getMissingFragments")
    for(uint32_t i = 0; i <= lastfragmentNum; i++){ //lastfragmentNum
        // logDebug("[ChangeForWriter] checking: " << unsigned(sequenceNumber) << " " << unsigned(i))
        auto frag = sampleFragmentArray[i];
        if(!(frag->received)) {
            // logDebug("[ChangeForWriter] missing " << i)
            res->push_back(i);
        }
    }
}

}; // end namespace