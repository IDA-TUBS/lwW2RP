/*
 *
 */

#ifndef W2RP_WRITERPROXY_H_
#define W2RP_WRITERPROXY_H_

#include <math.h>
#include <list>
#include <w2rp/changeFromWriter.hpp>

namespace w2rp {

class WriterProxy
{
private:

    // history storing all changes for this writer proxy
    std::unordered_map<std::uint32_t, ChangeFromWriter> history;

    std::uint32_t latestSequenceNumber;

public:

    std::uint32_t writerID;

    std::uint32_t historySize;

    enum ChangeStatus{
        COMPLETE,
        INCOMPLETE,
        NOT_FOUND
    };


public:
    
    /** 
     * @brief Construct a new Writer Proxy object
     * 
     * @param id ID of the writer entity
     * @param historySize max size of history
    */
    WriterProxy(
        std::uint32_t id,
        std::uint32_t historySize
    );


    /**
     * @brief method for adding a new Cache Change to the proxy's history cache
     *
     * @param change reference to cache change that will be replicated in the reader proxy
     * @return true if change was added, false if cache was full and change was not added to history
     */
    bool addChange(CacheChange &change);

    /**
     * @brief method for updating the fragment status of a given change
     *
     * @param sequenceNumber sequence number of the change that shall be updated
     * @param fragmentNumber fragment number whose status shall be updated
     * @return ChangeStatus indicating whether the change is COMPLETE, INCOMPLETE or NOT_FOUND
     */
    ChangeStatus updateFragmentStatus(std::uint32_t sequenceNumber, std::uint32_t fragmentNumber);

    /**
     * @brief method for removing a Cache Change from the proxie's history cache
     *
     * @param sequenceNumber sequence number of the change that has be be removed
     */
    void removeChange(std::uint32_t sequenceNumber);

    /**
     * @brief method cache change with corresponding sequence number
     *
     * @param sequenceNumber seq number of sample that shall be retrieved
     * @return ChangeForWriter object
     */
    ChangeFromWriter& getChange(std::uint32_t sequenceNumber);

    /**
     * @brief check if the change corresponding to the specified sequence number has beend "published", i.e. passed to the reader for further processing
     * 
     * @param sequenceNumber 
     * @return true 
     * @return false 
     */
    bool checkPublished(std::uint32_t sequenceNumber);

    /**
     * @brief Set the published attribute for the change with the corresponding sequence number
     * 
     * @param sequenceNumber 
     */
    void setPublished(std::uint32_t sequenceNumber);

    /**
     * @brief Check if the change corresponding to the specified sequence number is complete
     * 
     * @param sequenceNumber 
     * @return true 
     * @return false 
     */
    bool isComplete(std::uint32_t sequenceNumber);

};

}; // end namespace

#endif // W2RP_WRITERPROXY_H_
