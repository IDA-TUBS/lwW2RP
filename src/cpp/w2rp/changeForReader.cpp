/*
 *
 */

#include <w2rp/changeForReader.h>
#include <w2rp/sampleFragment.h>

namespace w2rp {

uint32_t ChangeForReader::notAckCount()
{
    // count negatively acknowledged fragments and fragments that timed out
    // does not count fragments that have been sent but not acknowledged yet!
    uint32_t notAcked = 0;
    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if(!(frag->acked) && !(frag->sent))
        { //|| (frag->timeout && !(frag->acked))
            notAcked++;
        }
    }
    return notAcked;
}

uint32_t ChangeForReader::ackCount()
{
    // count acknowledged fragments
    uint32_t acked = 0;
    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if(frag->acked)
        {
            acked++;
        }
    }
    return acked;
}

uint32_t ChangeForReader::sentCount()
{
    // count sent fragments, includes already acked fragments as well!
    uint32_t sent = 0;
    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if(frag->sent || frag->acked)
        {
            sent++;
        }
    }
    return sent;
}

uint32_t ChangeForReader::unsentCount()
{
    // count unsent fragments
    uint32_t unsent = 0;
    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if(!(frag->sent))
        {
            unsent++;
        }
    }
    return unsent;
}

bool ChangeForReader::setFragmentStatus(fragmentStates status, uint32_t fragmentNumber, std::chrono::system_clock::time_point sentTimestamp)
{
    auto frag = sampleFragmentArray[fragmentNumber];

    switch(status)
    {
        case UNSENT:
            frag->sent = false;
            frag->acked = false;
            break;
        case SENT:
            lastSentFN = fragmentNumber;
            frag->sendTime = sentTimestamp;
            frag->sent = true;
            frag->acked = false;
            if(fragmentNumber > highestFNSend)
            {
                highestFNSend = fragmentNumber;
            }
            break;
        case ACKED:
            frag->sent = true;
            frag->acked = true;
            break;
        case NACKED:
            frag->sent = false;
            frag->acked = false;
            break;
        default:
            break;
    }

    return true;
}

std::vector<SampleFragment*> ChangeForReader::getUnsentFragments()
{
    std::vector<SampleFragment*> unsentFragments;

    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if(!(frag->sent) && !(frag->acked))
        {
            unsentFragments.push_back(frag);
        }
    }
    return unsentFragments;
}


void ChangeForReader::resetSentFragments()
{
    for(uint32_t i = 0; i < numberFragments; i++)
    {
        auto frag = sampleFragmentArray[i];
        if((frag->sent) && !(frag->acked))
        {
            auto now = std::chrono::system_clock::now();
            this->setFragmentStatus(UNSENT, frag->fragmentStartingNum, now);
        }
    }
}

}; // end namespace