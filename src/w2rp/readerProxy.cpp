#include <w2rp/readerProxy.hpp>
#include <tuple>

namespace w2rp {

ReaderProxy::ReaderProxy():
    readerID(0),
    historySize(0),
    timeout(),
    fragmentBudget(0),
    history()
{}

ReaderProxy::ReaderProxy(
    std::uint32_t id, 
    std::uint32_t historySize, 
    std::chrono::system_clock::duration nackSuppressionDuration,
    std::chrono::microseconds deadline,
    std::chrono::microseconds shapingTime,
    std::uint32_t agg_size
):
    readerID(id),
    historySize(historySize),
    nackSuppressionDuration(nackSuppressionDuration),
    timeout(),
    history()
{
    history.reserve(historySize);

    fragmentBudget = (deadline.count() / shapingTime.count()) * agg_size;
    logInfo("[ReaderProxy] initialized with fragment budget: " << fragmentBudget);
}

bool ReaderProxy::addChange(CacheChange &change)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    if(history.size() == historySize)
    {
        // cannot add a new change the history
        logError("[ReaderProxy] cannot add a new change the history")
        return false;
    }
    // Construct ChangeForReader in-place to avoid copying/moving the object
    // (it contains a mutex and is not move/copy constructible).
    history.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(change.sequenceNumber),
        std::forward_as_tuple(std::ref(change), readerID, fragmentBudget)
    );
    
    lock.unlock();
    return true;
}

ChangeForReader* ReaderProxy::getChange(std::uint32_t sequenceNumber)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    auto it = history.find(sequenceNumber);
    if(it != history.end())
    {
        lock.unlock();
        return &(it->second);
    }
    throw std::runtime_error("Change not found");
}

void ReaderProxy::removeChange(std::uint32_t sequenceNumber)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    auto it = history.find(sequenceNumber);
    if(it != history.end())
    {
        // Check if the change is incomplete
        if(it->second.checkForCompleteness() == false)
        {
            logInfo("[ReaderProxy] removing INCOMPLETE change " << it->second.sequenceNumber)
        }
        // Check if the removed change was being tracked for timeout
        if(timeout.active && timeout.sequenceNumber == sequenceNumber)
        {
            timeout.active = false;
            timeout.timestamp = std::chrono::system_clock::time_point();
            timeout.sequenceNumber = 0;
        }
        history.erase(it);
        lock.unlock();
        return;
    }
    else
    {
        logDebug("[ReaderProxy] change not found for removal")
        lock.unlock();
        return;
    }
}

ReaderProxy::changeStatus ReaderProxy::processNack(NackFrag *msg)
{
    std::unique_lock<std::mutex> lock(rp_mutex);

    // Get current time for nack supression checks
    std::chrono::system_clock::time_point t_now = std::chrono::system_clock::now();

    logDebug("[ReaderProxy] processNack() - received NackFrag: SN " << msg->writerSequenceNumber);

    bool set_nack = false;
    ChangeForReader* change = nullptr;
    auto change_it = history.find(msg->writerSequenceNumber);
    if(change_it != history.end())
    {
        change = &(change_it->second);
    }
    else
    {
        lock.unlock();
        return CHANGE_NOT_FOUND;
    }
    
    if(msg->fragmentStatus.base >= change->lowestNacked)
    {
        // Acknowledge all fragments up to base fragmentnumber - 1. Base is the lowest nacked fragment.
        change->setAcked(change->lowestNacked, msg->fragmentStatus.base-1);
    }

    for(std::uint32_t i = 0; i <= NACK_BITMAP_SIZE*8; i++)
    {
        std::uint32_t fragmentNum = msg->fragmentStatus.base + i;
        
        // Bitmap might cover more fragments than exist in the sample
        if(fragmentNum > change->fragmentStates.size())
        {
            break;
        }

        // Calculate the index of the byte containing the current bit
        std::size_t byte_index = i / 8;

        // Calculate the position of the bit within the byte
        std::size_t bit_position = 7 - (i % 8);
    
        if(!(msg->fragmentStatus.bitmap[byte_index] & (1 << bit_position)))
        {
            change->setAcked(fragmentNum);
        }
        else
        {
            int result = change->setNacked(fragmentNum, t_now, this->nackSuppressionDuration);
            if(result == 1)
            {
                return CHANGE_EXHAUSTED_RETRY_LIMIT;
            }
            set_nack = true;
        }
    }

    if(!set_nack) 
    {
        if(change->checkForCompleteness())
        {
            lock.unlock();
            return CHANGE_COMPLETE;
        }
    }

    lock.unlock();
    return CHANGE_INCOMPLETE;
}

bool ReaderProxy::checkSampleCompleteness(std::uint32_t sequenceNumber)
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    auto it = history.find(sequenceNumber);
    if(it == history.end())
    {
        logDebug("[ReaderProxy] change not found for completeness check");
        lock.unlock();
        return true; // consider not found samples as complete
    }
    else
    {
        lock.unlock();
        return it->second.checkForCompleteness();
    }
}

void ReaderProxy::setTimeout(std::chrono::system_clock::time_point timestamp, std::uint32_t sequenceNumber)
{
    this->timeout.sequenceNumber = sequenceNumber;
    this->timeout.active = true;
    this->timeout.timestamp = timestamp;
}

std::chrono::system_clock::time_point ReaderProxy::getTimeout()
{
    if(this->timeout.active)
    {
        return this->timeout.timestamp;
    }
    else
    {
        return std::chrono::system_clock::time_point();
    }
}

ReaderProxy::timeoutStatus ReaderProxy::checkTimeout()
{
    if(this->timeout.active)
    {
        if(this->timeout.timestamp <= std::chrono::system_clock::now())
        {
            return TIMEOUT_EXPIRED;
        }
        else
        {
            return TIMEOUT_PENDING;
        }
    }
    return NO_TIMEOUT;
}

bool ReaderProxy::resetTimedOutFragments()
{
    std::unique_lock<std::mutex> lock(rp_mutex);
    auto it = history.find(this->timeout.sequenceNumber);
    bool complete_status = false;
    if(it != history.end())
    {
        if(it->second.checkForCompleteness())
        {
            complete_status = false;
        }
        else
        {
            it->second.resetSentFragments();
            complete_status = true;
        }
    }
    else
    {
        logError("[ReaderProxy] change not found for timeout reset");
    }
    this->timeout.active = false;
    this->timeout.timestamp = std::chrono::system_clock::time_point();
    this->timeout.sequenceNumber = 0;
    lock.unlock();
    return complete_status;
}

}; // end namespace
