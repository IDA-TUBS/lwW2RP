#include <w2rp/writerProxy.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

WriterProxy::WriterProxy(
    std::uint32_t id,
    std::uint32_t historySize
):
    writerID(id), 
    historySize(historySize),
    latestSequenceNumber(0)
{}

bool WriterProxy::addChange(CacheChange &change)
{
    // Per RTPS spec, sequence numbers should start with 1, thus 0 is a valid initial value
    // However, also checking that history is not empty still allows 0 as first sequence number
    if(change.sequenceNumber == latestSequenceNumber && history.size() > 0)
    {
        // no new sample, no need to add to history
        return false;
    }
    ChangeFromWriter cfw(change, writerID);

    history.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(change.sequenceNumber),
        std::forward_as_tuple(std::ref(change), writerID)
    );
    
    latestSequenceNumber = change.sequenceNumber;

    return true;
}

void WriterProxy::removeChange(uint32_t sequenceNumber)
{
    auto it = history.find(sequenceNumber);
    if(it != history.end())
    {
        logDebug("[WriterProxy] removing change " << sequenceNumber)
        if(it->second.isComplete() == false)
        {
            logInfo("[WriterProxy] removing INCOMPLETE change " << sequenceNumber)
        }
        history.erase(it);
    }
}

WriterProxy::ChangeStatus WriterProxy::updateFragmentStatus(
    std::uint32_t sequenceNumber, 
    std::uint32_t fragmentNumber
)
{
    auto it = history.find(sequenceNumber);
    if(it == history.end())
    {
        logDebug("[WriterProxy] change not found for fragment status update")
        return NOT_FOUND;
    }

    auto& change = it->second;
    change.markReceived(fragmentNumber);

    if(change.isComplete())
    {
        return COMPLETE;
    }
    else
    {
        return INCOMPLETE;
    }
}

ChangeFromWriter& WriterProxy::getChange(std::uint32_t sequenceNumber)
{
    return history.at(sequenceNumber);
}

bool WriterProxy::checkPublished(std::uint32_t sequenceNumber)
{
    ChangeFromWriter& change = history.at(sequenceNumber);
    return change.isPublished();
}

void WriterProxy::setPublished(std::uint32_t sequenceNumber)
{
    ChangeFromWriter& change = history.at(sequenceNumber);
    change.setPublished();

}

bool WriterProxy::isComplete(std::uint32_t sequenceNumber)
{
    ChangeFromWriter& change = history.at(sequenceNumber);
    return change.isComplete();
}


}; // end namespace