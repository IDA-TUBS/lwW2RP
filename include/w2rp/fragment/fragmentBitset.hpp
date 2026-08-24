#ifndef W2RP_FRAGMENT_FRAGMENTBITSET_H_
#define W2RP_FRAGMENT_FRAGMENTBITSET_H_

#include <vector>
#include <cstdint>

#include <w2rp/log.hpp>
#include <bitset>
#include <sstream>

namespace w2rp {

class FragmentBitset
{
public:
    
    /**
     * @brief Completeness status
     * 
     */
    enum class Completeness {
        Complete = 1,
        Incomplete = 0,
        Error = -1 
    };

    /**
     * @brief Construct a new Fragment Bitset object
     * 
     * @param numFragments 
     */
    FragmentBitset(std::size_t numFragments): 
        bitset((numFragments + 7) / 8, 0xFF),
        size((numFragments + 7) / 8),
        sizeBits(numFragments),
        next_unsent(1)
    {
        // Mask off unused bits in the last byte so they are treated as 'received' (0)
        std::size_t validBits = numFragments % 8;
        if (validBits != 0) {
            uint8_t mask = uint8_t(0xFF >> validBits);
            bitset.back() &= ~mask;
        }
    }

    /**
     * @brief Unset the bit for the specified fragment number (from 1 to 0). Note: 1 = NACK and 0 = ACK as per RTPS specification.
     * 
     * @param fragmentNumber 
     * @return Completeness Reception status of the fragment set for the mapped sample
     */
    Completeness unset(std::size_t fragmentNumber) {
        // mark fragment as 'ack' (received) -> bit = 0
        if (fragmentNumber >= 1u && fragmentNumber <= sizeBits) {
            std::size_t pos = fragmentNumber - 1; // zero-based
            std::size_t byteIndex = pos / 8;
            std::size_t bitIndex = 7 - (pos % 8);
            uint8_t mask = uint8_t(1u << bitIndex);
            bitset[byteIndex] &= ~mask;

            // if we just cleared the current next_unsent, advance it to the next 1-bit
            if (fragmentNumber == next_unsent) {
                // find next missing fragment (1-based result)
                std::size_t i = fragmentNumber + 1;
                for (; i <= sizeBits; ++i) {
                    if (get(i) == 1) { // missing
                        break;
                    }
                }
                if(i <= sizeBits){
                    next_unsent = i;
                    return Completeness::Incomplete;
                }
                else
                {
                    // all fragments received
                    next_unsent = sizeBits + 1;
                    return Completeness::Complete;
                }
            }
            else
            {
                if(next_unsent <= sizeBits)
                {
                    return Completeness::Incomplete;
                }
                else
                {
                    return Completeness::Complete;
                }
            }
        }
        logError("[fragmentBitset] unset() - Error fragment Number: " << fragmentNumber << " out of range: " << sizeBits)
        return Completeness::Error;
    }

    /**
     * @brief get the ack/nack bit value for the specified fragment number
     * 
     * @param fragmentNumber 
     * @return int 
     */
    int get(std::size_t fragmentNumber) const 
    {
        // external API is 1-based
        if (fragmentNumber >= 1 && fragmentNumber <= sizeBits) {
            std::size_t pos = fragmentNumber - 1;
            std::size_t byteIndex = pos / 8;
            std::size_t bitIndex = 7 - (pos % 8);
            return (bitset[byteIndex] & (1u << bitIndex)) ? 1 : 0;
        }
        return -1;
    }

    /**
     * @brief Get the Size object
     * 
     * @return std::size_t 
     */
    std::size_t getSize() const {
        return size;
    }

    /**
     * @brief Return index (1-based) of the first missing fragment (bit==1), or size+1 if none.
     * NOTE: External API uses 1-based fragment numbers to match the rest of the codebase.
     */
    std::size_t getNextUnsent() const {
        return next_unsent;
    }

    /**
     * @brief Get the Bitset Vector object
     * 
     * @return std::vector<uint8_t>& 
     */
    std::vector<uint8_t>& getBitsetVector() {
        return bitset;
    }

    /**
     * @brief Get the (raw) pointer to the bitset vector
     * 
     * @return std::uint8_t* 
     */
    std::uint8_t* getBitsetData() {
        return bitset.data();
    }

    /**
     * @brief Get the (raw) pointer to the bitset vector starting at the byte index for the specified fragment Number.
     * 
     * @param fragmentNumber 
     * @return std::uint8_t* 
     */
    std::uint8_t* getBitsetData(std::size_t fragmentNumber)
    {
        std::size_t pos = fragmentNumber - 1;
        std::size_t byteIndex = pos / 8;
        return bitset.data() + byteIndex;
    }

    std::size_t getBitsetBase(std::size_t fragmentNumber)
    {
        std::size_t pos = fragmentNumber - 1;
        // The base value must be expressed in multiples of 8.
        // For fragment number 15, the base is 8 since floor(15/8) = 1 and 1*8 = 8, and so on. 
        std::size_t byteIndex = pos / 8; 
        return byteIndex * 8 + 1;
    }

    /**
     * @brief Get the number of bits (corresponding to the number of fragments) managed by the fragmentBitSet
     * 
     * @return std::size_t 
     */
    std::size_t getSizeBits()
    {
        return sizeBits;
    }

private:
    std::vector<uint8_t> bitset;
    std::size_t size;
    std::size_t sizeBits;
    std::size_t next_unsent;

};

}; // end namespace w2rp



#endif // W2RP_FRAGMENT_FRAGMENTBITSET_H_