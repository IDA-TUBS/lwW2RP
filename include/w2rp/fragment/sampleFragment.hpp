#ifndef W2RP_ENTITIES_SAMPLEFRAGMENT_H_
#define W2RP_ENTITIES_SAMPLEFRAGMENT_H_

#include <chrono>
#include <cstring>
#include <memory>
#include <cstdint>

#include <w2rp/log.hpp>

namespace w2rp {

class CacheChange;

class SampleFragment
{
     public:
    // fragment number
    std::uint32_t number;

    // actual fragment size (can be smaller than fragmentSize)
    std::uint32_t size;

    // fragment data
    std::vector<unsigned char> payload;

    /**
     * @brief empty default constructor
     */
    SampleFragment():
        number(0),
        size(0),
        payload()
    {};

    /**
     * @brief overloaded constructor, creates sampleFragment without data
     *
     * @param fragNum fragment number
     * @param dataSize size of the fragment in bytes
     */
    SampleFragment(unsigned int fragNum, unsigned int dataSize):
        number(fragNum),
        size(dataSize),
        payload()
    {
        payload.resize(dataSize);
    };

    /**
     * @brief overloaded constructor, creates sampleFragment with data
     *
     * @param fragNum fragment number
     * @param dataSize size of the fragment in bytes
     * @param binaryData binary representation of the fragment data
     */
    SampleFragment(unsigned int fragNum, unsigned int dataSize, unsigned char* binaryData):
        number(fragNum),
        size(dataSize),
        payload(binaryData, binaryData + dataSize)
    {

    };

    /**
     * @brief Disable Copy Constructor
     */
    SampleFragment(const SampleFragment&) = delete;

    /**
     * @brief Move constructor
     */
    SampleFragment(SampleFragment&&) noexcept = default;

    /**
     * @brief Move assignment
     */
    SampleFragment& operator=(SampleFragment&&) noexcept = default;

    /**
     * @brief empty default destructor
     */
    ~SampleFragment() = default;

    /**
     * @brief comparison operator
     */
    bool operator == (const SampleFragment& other) const
    {
        return ((this->size == other.size) &&
               (0 == memcmp(this->payload.data(), other.payload.data(), this->size)) &&
               (this->number == other.number));
    }   

    /**
     * @brief set data
     *
     * @param binaryData binary representation of the fragment data 
     */
    void setData(unsigned char* binaryData, std::uint32_t dataSize, std::uint32_t fragmentNum)
    {
        this->number = fragmentNum;
        this->size = dataSize;
        this->payload.resize(dataSize);
        memcpy(this->payload.data(), binaryData, dataSize);
    };

};

}; // end namespace

#endif // W2RP_ENTITIES_SAMPLEFRAGMENT_H_
