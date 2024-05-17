/*
 *
 */

#include <w2rp/writerProxy.hpp>

namespace w2rp {

bool WriterProxy::addChange(CacheChange &change)
{
    if(change.sequenceNumber == highestSequenceNumber) // || history.size() == historySize)
    {
        // no new sample, no need to add to history
        return false;
    }

    ChangeForWriter* cfr = new ChangeForWriter(change);

    history.push_back(cfr);
    highestSequenceNumber = change.sequenceNumber;
    return true;
}

void WriterProxy::removeChange(uint32_t sequenceNumber)
{
    for (auto it = history.begin(); it != history.end();)
    {
        if ((*it)->sequenceNumber <= sequenceNumber)
        {

            auto change = (*it);
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


bool WriterProxy::updateFragmentStatus(fragmentStates status, uint32_t sequenceNumber, uint32_t fragmentNumber, unsigned char *data, uint32_t dataLength)
{
    // first find change corresponding to the given sequence number
    ChangeForWriter* tmp = nullptr;
    for (auto cfw: history)
    {
        if (cfw->sequenceNumber == sequenceNumber)
        {
            tmp = cfw;
            break;
        }
    }

    if(tmp)
    {
        bool status_state = tmp->setFragmentStatus(status, fragmentNumber);
        bool status_data = tmp->setFragmentData(fragmentNumber, data, dataLength);

        return (status_data && status_state);
    }
 
    return false;
}


bool WriterProxy::checkSampleCompleteness(uint32_t sequenceNumber)
{
    // access change with the given sequence number
    ChangeForWriter* change = nullptr;
    for (auto cfw: history)
    {
        if (cfw->sequenceNumber == sequenceNumber)
        {
            change = cfw;
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
        // remove sample from history? or do at a different point in time
        // done elsewhere
    }

    return complete;
}


ChangeForWriter* WriterProxy::getChange(uint32_t sequenceNumber)
    {
        // first find change corresponding to the given sequence number
        ChangeForWriter* tmp = nullptr;
        for (auto cfw: history)
        {
            if (cfw->sequenceNumber == sequenceNumber)
            {
                tmp = cfw;
                break;
            }
        }

        return tmp;
    };

}; // end namespace