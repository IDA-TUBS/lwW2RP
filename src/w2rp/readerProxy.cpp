/*
 *
 */

#include <w2rp/readerProxy.hpp>
#include <w2rp/config/config.hpp>
#include <w2rp/log.hpp>

#include <bitset>

namespace w2rp {

bool ReaderProxy::addChange(CacheChange &change)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    // logDebug("[ReaderProxy] adding change")
    if(history.size() == historySize)
    {
        // cannot add a new change the history
        // logDebug("[ReaderProxy] cannot add a new change the history")
        return false;
    }
    ChangeForReader* cfr = new ChangeForReader(this->readerID, change);

    history.push_back(cfr);
    // logDebug("[ReaderProxy] adding change complete: " << history.size())
    return true;
    lock.unlock();
}

void ReaderProxy::removeChange(uint32_t sequenceNumber)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    for (auto it = history.begin(); it != history.end();)
    {
        if ((*it)->sequenceNumber <= sequenceNumber)
        {
            // logDebug("[ReaderProxy] removing change: " << (*it)->sequenceNumber)
            if((*it)->getCompleteFlag() == false)
            {
                logInfo("[ReaderProxy] removing INCOMPLETE change " << (*it)->sequenceNumber)

            }
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
    lock.unlock();
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
    std::unique_lock<std::mutex> lock(rp_mutex);
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
    lock.unlock();

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
        // logDebug("[ReaderProxy] history empty")
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
        // logDebug("[ReaderProxy] change empty")
        return false;
    }

    FragmentNumberSet bitmapStruct = msg->fragmentNumberState;
    unsigned char bitmap[NACK_BITMAP_SIZE];
    memcpy(bitmap, bitmapStruct.bitmap, NACK_BITMAP_SIZE);
    uint32_t smallestFN = bitmapStruct.bitmapBase;
    uint32_t highestFN = msg->count;

    // logDebug("[ReaderProxy] min FN: " << unsigned(smallestFN) << " max FN: " << unsigned(highestFN))

    // iterate over all relevant bits of the NackFrag bitmap
    for(uint32_t i = 0; i <= highestFN - smallestFN; i++)
    {
        // first retrieve corresponding fragment from readerproxy's history
        uint32_t fragmentNum = smallestFN + i;
        SampleFragment* currentFragment = change->getFragmentArray()[fragmentNum];

        if(currentFragment->acked){
            // skip processing of already acknowledged fragments
            // logDebug("[ReaderProxy] Fragment " << currentFragment->fragmentStartingNum << " acked")
            continue;
        }
        
        // logDebug("[ReaderProxy] Fragment " << currentFragment->fragmentStartingNum << " not acked")

        // Calculate the index of the byte containing the current bit
        int byte_index = i / 8;
        // Calculate the position of the bit within the byte
        int bit_position = 7 - (i % 8);

        // int bit_position = i % 8;

        // logDebug("[ReaderProxy] byte index: " << byte_index << " bit position: " << bit_position) 

        // Check if the bit is set
        if (!(bitmap[byte_index] & (1 << bit_position))) { //!(bitmap[byte_index] & (1 >> bit_position)
            // fragment not noted as missing -> consider as acked
            // logDebug("[ReaderProxy] Fragment " << currentFragment->fragmentStartingNum << " update acked")
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

    // Create a string to hold the bitstream
    std::string bitstream;

    // Convert each byte to a bit representation and append it to the string
    int iterations = (highestFN - smallestFN)/8 + 1;
    for (std::size_t i = 0; i < iterations; ++i) {
        bitstream += std::bitset<8>(bitmap[i]).to_string();  // Convert byte to bitstring
    }
    logTrace("NACKFRAG,SN," << sequenceNumber << ",smallestFN," << smallestFN << ",highestFN," << highestFN << ",bitmap," << bitstream)


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

    std::unique_lock<std::mutex> lock(rp_mutex);
    std::vector<SampleFragment*> unsentFragments;
    if(change)
    {
        unsentFragments = change->getUnsentFragments();
    }
    lock.unlock();

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
        std::unique_lock<std::mutex> lock(rp_mutex);
        complete = change->checkForCompleteness();
        lock.unlock();
    }
    if(complete)
    {
        // remove sample from history? maybe just remove if expired or all readers completed reception of a given sample
        // done elsewhere
        // TODO really??
        // logDebug("[ReaderProxy] sample acknowledged completely.\n----------------------------------------------------------------------------------------------")
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
        std::unique_lock<std::mutex> lock(rp_mutex);
        change->resetSentFragments();
        lock.unlock();
    }
}


uint32_t ReaderProxy::getAckCount(uint32_t sequenceNumber)
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
        return change->ackCount();
    }
    else
    {
        // TODO handle problem
        return 0;
    }
    
}


}; // end namespace