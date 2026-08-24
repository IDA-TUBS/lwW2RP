#ifndef W2RP_CHANGEFROMWRITER_H_
#define W2RP_CHANGEFROMWRITER_H_

#include <w2rp/cacheChange.hpp>
#include <w2rp/fragment/fragmentBitset.hpp>


namespace w2rp {

class ChangeFromWriter
{
private:
    // reader entity ID
    std::uint32_t readerID;

    FragmentBitset bitmap;

    bool complete;

    bool published;

public:

    std::uint32_t sequenceNumber;

    std::uint32_t lastReceivedFN;

    std::uint32_t highestFNreceived;

public:
    
    /** 
     * @brief Construct a new Change From Writer object
     * 
     * @param change reference to cache change
     * @param readerID ID of the reader entity
    */
    ChangeFromWriter(CacheChange &change, std::uint32_t readerID);

    /**
     * @brief method for marking a fragment as received
     * 
     * @param fragmentNumber number of fragment that was received
     */
    void markReceived(std::uint32_t fragmentNumber);

    /**
     * @brief method for checking whether all fragments have been received
     * 
     * @return true: complete, false: incomplete
     */
    bool isComplete();

    /**
     * @brief Method for checking if the sample has been "published" to the reader for further processing.
     * 
     * @return true 
     * @return false 
     */
    bool isPublished();

    /**
     * @brief Set the Published attribute
     * 
     */
    void setPublished();

    /**
     * @brief Get the First Missing Fragment number
     * 
     * @return std::uint32_t 
     */
    std::uint32_t getFirstMissingFragment();

    /**
     * @brief Get a (raw) pointer to the missing fragment map 
     * 
     * @return std::uint8_t* 
     */
    std::uint8_t* getMissingFragmentMap();

    /**
     * @brief Get the base fragment number for the current missing fragment map
     * 
     * @return std::uint32_t 
     */
    std::uint32_t getMissingFragmentMapBase();

    /**
     * @brief Get the number of fragments for the mapped sample
     * 
     * @return std::uint32_t 
     */
    std::uint32_t getFragmentCount();
};

}; // end namespace

#endif // W2RP_CHANGEFROMWRITER_H_
