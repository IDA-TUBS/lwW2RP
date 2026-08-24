#include <w2rp/changeForReader.hpp>

namespace w2rp {

ChangeForReader::ChangeForReader():
    complete(false),
    transmittedAll(false),
    lastSent(0),
    highestSent(0),
    lowestNacked(0)
{}

ChangeForReader::ChangeForReader(
    CacheChange &change, 
    std::uint32_t id,
    std::uint32_t fragment_budget
):
    readerID(id),
    complete(false),
    transmittedAll(false),
    lastSent(0),
    highestSent(0),
    lowestNacked(1),
    sequenceNumber(change.sequenceNumber)
{
    std::uint32_t numFragments = change.numberFragments;
    retransmissionBudget = fragment_budget - numFragments;
    fragmentStates.reserve(numFragments);
    for(std::uint32_t i = 1; i <= numFragments; i++)
    {   
        fragmentStates.emplace_back(change.getFragment(i));
        initialQueue.push_back(i);
    }
}

ChangeForReader::~ChangeForReader()
{}

std::size_t ChangeForReader::unsentCount()
{
    return unsentQueue.size() + initialQueue.size();
}

std::size_t ChangeForReader::retransmissionBudgetCount()
{
    return retransmissionBudget;
}

void ChangeForReader::resetSentFragments()
{
    std::unique_lock<std::mutex> lock(cache_mutex);

    // Read all sent fragment numbers into a temporary vector and clear the sent set
    std::vector<std::uint32_t> fragmentsToReset(sentSet.begin(), sentSet.end());
    sentSet.clear();
    
    logDebug("[ChangeForReader] resetSentFragments() - resetting fragments: " << fragmentsToReset.size() << ", retransmission budget: " << retransmissionBudget)

    SampleFragmentState::FragmentStatus status;

    // Reset all sent fragments to unsent
    for(std::uint32_t fragmentNum : fragmentsToReset)
    {
        status = fragmentStates[fragmentNum-1].getStatus();
        // Only push to unsent queue if fragment is in state sent
        logDebug("[ChangeForReader] resetSentFragments() - resetting fragment: " << fragmentNum << ", status: " << status)
        if(status == SampleFragmentState::SENT)
        {
            if(!fragmentStates[fragmentNum-1].isScheduledForTx() && retransmissionBudget > 0)
            {
                unsentQueue.push(fragmentNum);
                fragmentStates[fragmentNum-1].setUnsent();
                logDebug("[ChangeForReader] resetSentFragments() - fragment reset: " << fragmentNum)
                fragmentStates[fragmentNum-1].setScheduledForTx(true);
                retransmissionBudget--;
            }
            else
            {
                logDebug("[ChangeForReader] resetSentFragments() - fragment not reset: " << fragmentNum)
                // Return Fragment to sentSet if there is no retransmission budget left
                sentSet.emplace(fragmentNum);
            }
        }
    }
}

bool ChangeForReader::checkForCompleteness()
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    if(!complete)
    {
        // check if complete flag can be set
        if(unsentQueue.empty() && initialQueue.empty() && sentSet.empty())
        {
            this->complete = true;
            logDebug("[ChangeForReader] checkForCompleteness() - change " << sequenceNumber << " is COMPLETE")
            lock.unlock();
        }
    }
    return this->complete;
}

std::uint32_t ChangeForReader::getFragmentForTx()
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    std::uint32_t fragmentNumber = 0;
    
    // Transmission logic: first send all fragments that have never been sent before
    if(!transmittedAll)
    {
        fragmentNumber = initialQueue.front();
        initialQueue.pop_front();
        if(initialQueue.empty())
        {
            transmittedAll = true;
        }
    }
    // Retransmission Phase: Then send fragments from the unsent queue
    else
    {
        /************************************************************************* 
            find next fragment in unsent queue that is scheduled for transmission 
            Note: The "isScheduledForTx" flag is used to avoid sending fragments
            that have been acknowledged after being pushed to the unsent queue.
            This situation can occur due to the asynchronous nature of NACK
            processing and fragment transmission.
        **************************************************************************/
        while(!unsentQueue.empty())
        {
            fragmentNumber = unsentQueue.top();
            unsentQueue.pop();
            if(fragmentStates[fragmentNumber-1].isScheduledForTx())
            {
                logDebug("[changeForReader] getFragmentForTx() - retransmitting fragment: " << fragmentNumber)
                break;
            }
            else
            {
                // Fragment no longer scheduled for transmission, continue to next fragment in queue
                fragmentNumber = 0;
            }
        }

        // If no fragment is found in the unsent queue, return 0
        if(unsentQueue.empty() && fragmentNumber == 0)
        {
            lock.unlock();
            return 0;
        }
    }

    sentSet.emplace(fragmentNumber);
    fragmentStates[fragmentNumber-1].setSent();
    // Mark fragment as no longer scheduled for transmission
    fragmentStates[fragmentNumber-1].setScheduledForTx(false);

    lastSent = fragmentNumber;

    if(fragmentNumber > highestSent)
    {
        highestSent = fragmentNumber;
    }
    lock.unlock();
    return fragmentNumber;
}

bool ChangeForReader::setAcked(std::uint32_t fragmentNumber)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    SampleFragmentState::FragmentStatus status = fragmentStates[fragmentNumber-1].getStatus();
    if(fragmentNumber == 0 || fragmentNumber > fragmentStates.size())
    {
        lock.unlock();
        return false;
    }
    else if(status != SampleFragmentState::FragmentStatus::ACKED)
    {
        logDebug("[ChangeForReader] setAcked() single -  Acknowledging fragment " << fragmentNumber)
        fragmentStates[fragmentNumber-1].setAcked();

        if(fragmentStates[fragmentNumber-1].isScheduledForTx())
        {
            fragmentStates[fragmentNumber-1].setScheduledForTx(false);
            retransmissionBudget++;
            logDebug("[ChangeForReader] setAcked() single -  Fragment " << fragmentNumber << "unscheduling. retransmission budget: " << retransmissionBudget)
        }

        if(lowestNacked == fragmentNumber)
        {
            lowestNacked++;
        }
        sentSet.erase(fragmentNumber);
    }
    lock.unlock();
    return true;
}

bool ChangeForReader::setAcked(
    std::uint32_t lowestFragmentNumber,
    std::uint32_t highestFragmentNumber
)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    if(lowestFragmentNumber == 0 || highestFragmentNumber > fragmentStates.size() || lowestFragmentNumber > highestFragmentNumber)
    {
        lock.unlock();
        return false;
    }

    for(std::uint32_t fn = lowestFragmentNumber; fn <= highestFragmentNumber; fn++)
    {
        if(fragmentStates[fn-1].getStatus() != SampleFragmentState::FragmentStatus::ACKED)
        {
            logDebug("[ChangeForReader] setAcked() multi -  Acknowledging fragment " << fn)
            fragmentStates[fn-1].setAcked();
            if(fragmentStates[fn-1].isScheduledForTx())
            {
                fragmentStates[fn-1].setScheduledForTx(false);
                retransmissionBudget++;
                logDebug("[ChangeForReader] setAcked() multi -  Fragment " << fn << "unscheduling. retransmission budget: " << retransmissionBudget)
            }
            fragmentStates[fn-1].setScheduledForTx(false);
            sentSet.erase(fn);
        }
    }

    lowestNacked = highestFragmentNumber + 1;
    lock.unlock();
    return true;
}

int ChangeForReader::setNacked(
    std::uint32_t fragmentNumber,
    std::chrono::system_clock::time_point nackTimestamp,
    std::chrono::system_clock::duration nackSuppressionInterval
)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    if(fragmentNumber == 0 || fragmentNumber > fragmentStates.size())
    {
        lock.unlock();
        return -1;
    }

    auto lastSentTime = fragmentStates[fragmentNumber-1].getSentTime();
    if((nackTimestamp - lastSentTime) < nackSuppressionInterval)
    {
        // Nack suppression active, do not set to NACKED
        lock.unlock();
        logDebug("[ChangeForReader] setNacked() - Nack suppression active for fragment " << fragmentNumber)
        return 0;
    }

    // Get current fragment status
    SampleFragmentState::FragmentStatus status = fragmentStates[fragmentNumber-1].getStatus();

    fragmentStates[fragmentNumber-1].setNacked();

    logDebug("[ChangeForReader] setNacked() - Nacking fragment " << fragmentNumber << ", nack suppression: " << ((status == SampleFragmentState::FragmentStatus::SENT) ? "false" : "true"))

    // Only push to unsent queue if fragment is in state sent
    // Nack might have been received after timeout expired and is already reset to unsent!
    if(status == SampleFragmentState::SENT)
    {
        if(retransmissionBudget == 0)
        {
            logDebug("[ChangeForReader] setNacked() - no retransmission budget left, fragment " << fragmentNumber << " not scheduled for retransmission")
            return 1;
        }
        else if(!fragmentStates[fragmentNumber-1].isScheduledForTx())
        {
            logDebug("[ChangeForReader] setNacked() - scheduling fragment " << fragmentNumber << " for retransmission, retransmission budget left: " << retransmissionBudget)
            unsentQueue.push(fragmentNumber);
            sentSet.erase(fragmentNumber);
            fragmentStates[fragmentNumber-1].setScheduledForTx(true);
            retransmissionBudget--;
        }
        else
        {
            // Fragment already scheduled for retransmission, do not push again to unsent queue
            logDebug("[ChangeForReader] setNacked() - fragment " << fragmentNumber << " already scheduled for retransmission")
        }
    }

    lock.unlock();
    return 0;
}

}; // end namespace