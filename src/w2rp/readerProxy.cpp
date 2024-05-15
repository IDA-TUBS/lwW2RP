/*
 *
 */

#include <w2rp/readerProxy.hpp>

namespace w2rp {

bool ReaderProxy::addChange(CacheChange &change)
{
    if(history.size() == historySize)
    {
        // cannot add a new change the history
        return false;
    }
    ChangeForReader* cfr = new ChangeForReader(this->readerID, change);

    history.push_back(cfr);
    return true;
}

void ReaderProxy::removeChange(uint32_t sequenceNumber)
{

    for (auto it = history.begin(); it != history.end();)
    {
        if ((*it)->sequenceNumber <= sequenceNumber)
        {
            ChangeForReader* change = (*it);
            history.erase(it);
            delete change;
            return;
        }
        else
        {
            ++it;
        }
    }
}

bool ReaderProxy::changeExists(uint32_t sequenceNumber)
{
    for (auto it = history.begin(); it != history.end();)
    {
        if ((*it)->sequenceNumber <= sequenceNumber)
        {
            return true;
        }
        else
        {
            ++it;
        }
    }

    return false;
}


bool ReaderProxy::updateFragmentStatus (fragmentStates status, uint32_t sequenceNumber, uint32_t fragmentNumber, std::chrono::system_clock::time_point sentTimestamp)
{
    // first find change corresponding to the given sequence number
    ChangeForReader* tmp = nullptr;
    for (auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            tmp = cfr;
            break;
        }
    }

    return tmp->setFragmentStatus(status, fragmentNumber, sentTimestamp);
}

bool ReaderProxy::processNack(NackFrag *msg) 
{
    auto t_now = std::chrono::system_clock::now();
    // to be called by writer NackFrag reception callback
    
    // first get relevant sequence number
    uint32_t sequenceNumber = msg->writerSN;

    if(history.size() == 0)
    {
        return false;
    }

    // access change with the given sequence number
    ChangeForReader* change = nullptr;
    for(auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            change = cfr;
            break;
        }
    }
    if(!change)
    {
        return false;
    }

    FragmentNumberSet bitmapStruct = msg->fragmentNumberState;
    unsigned char bitmap[8];
    memcpy(bitmap, bitmapStruct.bitmap, 8);
    uint32_t smallestFN = bitmapStruct.bitmapBase;
    uint32_t highestFN = msg->count;

    // iterate over all relevant bits of the NackFrag bitmap
    for(uint32_t i = 0; i <= highestFN - smallestFN; i++)
    {
        // first retrieve corresponding fragment from readerproxy's history
        uint32_t fragmentNum = smallestFN + i;
        SampleFragment* currentFragment = change->getFragmentArray()[fragmentNum];

        if(currentFragment->acked){
            // skip processing of already acknowledged fragments
            continue;
        }
        
        // Calculate the index of the byte containing the current bit
        int byte_index = i / 8;
        // Calculate the position of the bit within the byte
        int bit_position = 7 - (i % 8);

        // Check if the bit is set
        if (!bitmap[byte_index] & (1 << bit_position)) {
            // fragment not noted as missing -> consider as acked
            this->updateFragmentStatus(ACKED, sequenceNumber, fragmentNum, t_now);
        } else {
            // fragment noted as missing

            // NackGuard
            if(this->nackSuppressionEnabled && (currentFragment->sentTime + this->nackSuppressionDuration > std::chrono::system_clock::now()))
            {
                // do not process Nacks if NackGuard hasn't expired yet
                continue;
            }

            if(currentFragment->sent){
                this->updateFragmentStatus(NACKED, sequenceNumber, fragmentNum, t_now);
            }
        }
    }

    return true;
}


std::vector<SampleFragment*> ReaderProxy::getUnsentFragments(uint32_t sequenceNumber)
{
    // access change with the given sequence number
    ChangeForReader* change = nullptr;
    for (auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            change = cfr;
            break;
        }
    }

    std::vector<SampleFragment*> unsentFragments = change->getUnsentFragments();

    return unsentFragments;
}


bool ReaderProxy::checkSampleCompleteness(uint32_t sequenceNumber)
{
    // access change with the given sequence number
    ChangeForReader* change = nullptr;
    for (auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            change = cfr;
            break;
        }
    }

    bool complete = false;
    if(change)
    {
        complete = change->checkForCompleteness();
    }
    if(complete)
    {
        // remove sample from history? maybe just remove if expired or all readers completed reception of a given sample
        // done elsewhere
    }
    return complete;
}


bool ReaderProxy::checkForTimeout(uint32_t sequenceNumber)
{
    // access change with the given sequence number
    ChangeForReader* change = nullptr;
    for (auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            change = cfr;
            break;
        }
    }

    if(change)
    {
        // timeout necessary if no unsent fragments remain but change has not been acknowledged in its entirety yet
        if(!(change->complete) && (change->unsentCount() == 0) && (change->ackCount() != change->numberFragments))
        {
            return true;
        }
    }
    return false;
}

void ReaderProxy::resetTimeoutedFragments(uint32_t sequenceNumber)
{
    // access change with the given sequence number
    ChangeForReader* change = nullptr;
    for (auto cfr: history)
    {
        if (cfr->sequenceNumber == sequenceNumber)
        {
            change = cfr;
            break;
        }
    }

    if(change && !(change->complete))
    {
        change->resetSentFragments();
    }
}

}; // end namespace