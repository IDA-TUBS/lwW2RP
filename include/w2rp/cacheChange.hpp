#ifndef W2RP_CACHECHANGE_H_
#define W2RP_CACHECHANGE_H_

#include <math.h>
#include <chrono>
#include <vector>
#include <cstdint>

#include <w2rp/fragment/sampleFragment.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

using SerializedPayload = std::vector<unsigned char>;

class SampleFragment;

class CacheChange
{
    
    /* --------------------------------- Methods -------------------------------- */
    public:
    /*
     * empty default constructor
     */
    CacheChange();

    /**
     * @brief Construct a new Cache Change object (Writer)
     *
     * @param sample reference to the serialized payload (sample) 
     * @param seqNum sequence number of the current sample
     * @param sampleSize size of the sample in bytes
     * @param fragmentSize size of a fragment in bytes
     * @param timestamp time when sample was generated/arrived at middleware
     * @param lifeTime lifetime of the sample, i.e., time until sample is considered outdated
     */
    CacheChange(
        SerializedPayload& sample,
        std::uint32_t seqNum, 
        std::uint32_t sampleSize, 
        std::uint32_t fragmentSize, 
        std::chrono::system_clock::time_point timestamp,
        std::chrono::microseconds lifeTime
    );

    /**
     * @brief Construct a new Cache Change object (Reader)
     *
     * @param seqNum sequence number of the current sample
     * @param sampleSize size of the sample in bytes
     * @param fragmentSize size of a fragment in bytes
     * @param timestamp time when sample was generated/arrived at middleware
     */
    CacheChange(
        std::uint32_t seqNum, 
        std::uint32_t sampleSize, 
        std::uint32_t fragmentSize, 
        std::chrono::system_clock::time_point timestamp
    );

    /**
     * @brief Disable copy constructor
     */
    CacheChange(const CacheChange &change) = delete;

    /**
     * @brief Move constructor
     */
    CacheChange(CacheChange &&other) noexcept;

    /**
     * @brief Move assignment
     */
    CacheChange& operator=(CacheChange &&other) noexcept;

    /**
     * @brief destructor
     */
    ~CacheChange();

    /**
     * @brief determine whether age has been exceeded
     *
     * @return true if sample still valid, else returns false
     */
    bool isValid();

    /**
     * @brief Get the Fragment object   
     * 
     * @param fragmentNumber 
     * @return SampleFragment& 
     */
    SampleFragment* getFragment(std::uint32_t fragmentNumber);

    /**
     * @brief method for updating fragment data
     * 
     * @param fragmentNumber fragment number to be updated
     * @param data pointer to new fragment data
     * @param dataLength length of new fragment data
     */
    void updateFragment(
        std::uint32_t fragmentNumber, 
        unsigned char* data, 
        std::uint32_t dataLength
    );

    /**
     * @brief method for serializing the entire sample into a SerializedPayload object
     * 
     * @param sampleData reference to SerializedPayload object to store the serialized sample
     */
    void serializeSample(SerializedPayload& sampleData);


    private:
    /**
     * @brief method for fragmenting the sample into SampleFragment objects
     * 
     * @param payload pointer to SerializedPayload object containing the sample data
     */
    void fragmentSample(SerializedPayload& payload);


    /* --------------------------------- Attributes ----------------------------- */
public:

    // sequence number of current sample
    std::uint32_t sequenceNumber;
    
    // size of the sample in bytes
    std::uint32_t sampleSize;
    
    // size of each fragment
    std::uint32_t fragmentSize;
    
    // total number of fragments for the sample
    std::uint32_t numberFragments;

    // timestamp: when did the sample arrive at the middleware
    std::chrono::system_clock::time_point arrivalTime;

    // lifetime (deadline) of the sample, i.e., time until sample is considered outdated. 
    std::chrono::microseconds lifeTime;

private:

    // array storing the fragments
    std::vector<SampleFragment> sampleFragments;

    // mutex for object access
    std::mutex cache_mutex; 

    

};

}; // end namespace

#endif // W2RP_CACHECHANGE_H_
