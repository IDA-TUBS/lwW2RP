#include "w2rp/cacheChange.hpp"
#include <utility>

namespace w2rp {

CacheChange::CacheChange() {}

CacheChange::CacheChange(
    SerializedPayload& sample,
    std::uint32_t seqNum, 
    std::uint32_t sampleSize, 
    std::uint32_t fragmentSize, 
    std::chrono::system_clock::time_point timestamp,
    std::chrono::microseconds lifeTime
): 
    sequenceNumber(seqNum),
    sampleSize(sampleSize),
    fragmentSize(fragmentSize),
    numberFragments((sampleSize + fragmentSize - 1) / fragmentSize),
    arrivalTime(timestamp),
    lifeTime(lifeTime)
{
    fragmentSample(sample);
}

CacheChange::CacheChange(
    std::uint32_t seqNum, 
    std::uint32_t sampleSize, 
    std::uint32_t fragmentSize, 
    std::chrono::system_clock::time_point timestamp
): 
    sequenceNumber(seqNum),
    sampleSize(sampleSize),
    fragmentSize(fragmentSize),
    numberFragments((sampleSize + fragmentSize - 1) / fragmentSize),
    arrivalTime(timestamp),
    lifeTime(std::chrono::microseconds(0))
{
    sampleFragments.resize(numberFragments);
    // no fragmentation needed for reader-side CacheChange
}

CacheChange::~CacheChange()
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    sampleFragments.clear();
    lock.unlock();
}

// Move constructor
CacheChange::CacheChange(CacheChange &&other) noexcept:
    sequenceNumber(other.sequenceNumber),
    sampleSize(other.sampleSize),
    fragmentSize(other.fragmentSize),
    numberFragments(other.numberFragments),
    arrivalTime(other.arrivalTime),
    lifeTime(other.lifeTime),
    sampleFragments(std::move(other.sampleFragments))
{
    // leave other's internal state in a valid but unspecified state
    other.sequenceNumber = 0;
    other.sampleSize = 0;
    other.fragmentSize = 0;
    other.numberFragments = 0;
}

// Move assignment
CacheChange& CacheChange::operator=(CacheChange &&other) noexcept
{
    if(this != &other)
    {
        std::unique_lock<std::mutex> lock(cache_mutex);
        sequenceNumber = other.sequenceNumber;
        sampleSize = other.sampleSize;
        fragmentSize = other.fragmentSize;
        numberFragments = other.numberFragments;
        arrivalTime = other.arrivalTime;
        lifeTime = other.lifeTime;
        sampleFragments = std::move(other.sampleFragments);

        // leave other in a valid state
        other.sequenceNumber = 0;
        other.sampleSize = 0;
        other.fragmentSize = 0;
        other.numberFragments = 0;
    }
    return *this;
}

bool CacheChange::isValid()
{
    auto now = std::chrono::system_clock::now();
    return ((now - this->arrivalTime) < this->lifeTime);
}

void CacheChange::serializeSample(SerializedPayload &sampleData)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    for(auto& fragment : sampleFragments)
    {
        sampleData.insert(sampleData.end(), fragment.payload.begin(), fragment.payload.end());
    }
    lock.unlock();
}   

void CacheChange::fragmentSample(SerializedPayload& sample)
{
    std::uint32_t fragmentCount = (sample.size() + fragmentSize - 1) / fragmentSize;
    sampleFragments.clear();
    sampleFragments.reserve(fragmentCount);

    for (std::uint32_t fragNum = 1; fragNum <= fragmentCount; fragNum++)
    {
        std::uint32_t fragmentStart = fragmentSize * (fragNum - 1);
        std::uint32_t fragDataSize = fragNum < fragmentCount ? fragmentSize : sample.size() - fragmentStart;
        
        SampleFragment fragment(fragNum, fragDataSize, &(sample.at(fragmentStart)));
        sampleFragments.push_back(std::move(fragment));
    }
}

SampleFragment* CacheChange::getFragment(std::uint32_t fragmentNumber)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    if(fragmentNumber == 0 || fragmentNumber > numberFragments)
    {
        logError("[CacheChange] getFragment: fragment number out of bounds")
        throw std::out_of_range("fragment number out of bounds");
    }
    return &sampleFragments[fragmentNumber - 1];
} 

void CacheChange::updateFragment(
    std::uint32_t fragmentNumber, 
    unsigned char* data, 
    std::uint32_t dataLength
)
{
    std::unique_lock<std::mutex> lock(cache_mutex);
    if(fragmentNumber == 0 || fragmentNumber > numberFragments)
    {
        logError("[CacheChange] updateFragment: fragment number out of bounds")
        throw std::out_of_range("fragment number out of bounds");
    }
    
    SampleFragment& fragment = sampleFragments[fragmentNumber - 1];
    if(fragment.size == 0)
    {
        fragment.setData(data, dataLength, fragmentNumber);
    }

    lock.unlock();
}

} // end namespace w2rp
