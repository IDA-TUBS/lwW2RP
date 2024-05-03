/*
 *
 */

#include <w2rp/changeForWriter.h>
#include <w2rp/sampleFragment.h>

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

}; // end namespace