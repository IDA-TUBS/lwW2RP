/*
 *
 */

#include <w2rp/writerProxy.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

bool WriterProxy::addChange(CacheChange &change)
{
    if(change.sequenceNumber == highestSequenceNumber) // || history.size() == historySize)
    {
        // no new sample, no need to add to history
        return false;
    }
    logDebug("[WriterProxy] added change " << change.sequenceNumber << " to history")
    ChangeForWriter* cfr = new ChangeForWriter(change);

    history.push_back(cfr);
    highestSequenceNumber = change.sequenceNumber;

    checkHistory();

    return true;
}

void WriterProxy::removeChange(uint32_t sequenceNumber)
{
    logDebug("[WriterProxy] remove change " << sequenceNumber)
    for (auto it = history.begin(); it != history.end();)
    {
        if ((*it)->sequenceNumber <= sequenceNumber)
        {
            logDebug("[WriterProxy] removing change " << (*it)->sequenceNumber)
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

void WriterProxy::checkHistory()
{
    if(history.size() > historySize)
    {
        logDebug("[WriterProxy] removing change " << history.front()->sequenceNumber << " " << std::boolalpha << history.front()->getCompleteFlag() << std::dec)
        delete history.front();
        history.pop_front();
    }
}

bool WriterProxy::checkChange(CacheChange &change)
{
    for (auto it = history.begin(); it != history.end(); it++)
    {
        if ((*it)->sequenceNumber == change.sequenceNumber)
        {
            return (*it)->getCompleteFlag();
        }
    }
    return false;
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