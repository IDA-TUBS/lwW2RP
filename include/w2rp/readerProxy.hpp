#ifndef W2RP_READERPROXY_H_
#define W2RP_READERPROXY_H_

#include <math.h>
#include <vector>
#include <list>
#include <bitset>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <mutex>

#include <w2rp/config/config.hpp>
#include <w2rp/helper/circularBuffer.hpp>
#include <w2rp/changeForReader.hpp>
#include <w2rp/messages/messages.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

class SampleFragment;

class ReaderProxy
{
    private:
        
    // history storing all changes for this reader proxy
    std::unordered_map<std::uint32_t, ChangeForReader> history; 
    
    // mutex for thread synchronization of history access
    std::mutex rp_mutex;

    // timeout info struct
    struct timeoutInfo 
    {
        timeoutInfo():
            active(false),
            timestamp(std::chrono::system_clock::time_point()),
            sequenceNumber(0)
        {};
        
        bool active;
        std::chrono::system_clock::time_point timestamp;
        std::uint32_t sequenceNumber;
    };
    timeoutInfo timeout;

    public:

    // entity of corresponding reader -> Simplification: host ID
    std::uint32_t readerID;  
    
    // max size of history
    std::uint32_t historySize;

    // NACK suppression duration
    std::chrono::system_clock::duration nackSuppressionDuration;

    // Maximum number of fragments that can be transmitted for one change. 
    // Used to determine the number of fragments that can be retransmitted within the deadline.
    std::uint32_t fragmentBudget;

    // timeout status enum
    enum timeoutStatus{
        NO_TIMEOUT,
        TIMEOUT_PENDING,
        TIMEOUT_EXPIRED
    };

    enum changeStatus
    {
        CHANGE_NOT_FOUND,
        CHANGE_INCOMPLETE,
        CHANGE_COMPLETE,
        CHANGE_EXHAUSTED_RETRY_LIMIT
    };

    /**
     * @brief Default empty constructor
     * 
    */
    ReaderProxy();
    
    /**
     * @brief Construct a new Reader Proxy object
     * 
     * @param id ReaderID
     * @param historySize Max size of history
     * @param nackSuppressionDuration NACK suppression duration
     * @param fragment_budget Maximum number of fragments that can be transmitted for one change.
     */
    ReaderProxy(
        std::uint32_t id, 
        std::uint32_t historySize, 
        std::chrono::system_clock::duration nackSuppressionDuration,
        std::chrono::microseconds deadline,
        std::chrono::microseconds shapingTime,
        std::uint32_t agg_size
    );

    /**
     * @brief Disable Copy Constructor
     */
    ReaderProxy(const ReaderProxy&) = delete;
    
    /**
     * @brief default destructor
     */
    ~ReaderProxy()
    {
        std::unique_lock<std::mutex> lock(rp_mutex);
        history.clear();
        lock.unlock();
    };

    /**
     * @brief Disable assignment operator
     * 
     */
    ReaderProxy& operator=(const ReaderProxy&) = delete;

    /**
     * @brief method for adding a new Cache Change to the proxy's history cache
     *
     * @param change reference to cache change that will be replicated in the reader proxy
     * @return true if change was added, false if cache was full and change was not added to history
     */
    bool addChange(CacheChange &change);

    /**
     * @brief Get the ChangeForReader for a specific sequence number
     * 
     * @param sequenceNumber 
     * @return ChangeForReader& 
     */
    ChangeForReader* getChange(std::uint32_t sequenceNumber);

    /**
     * @brief method for removing a Cache Change from the proxie's history cache
     *
     * @param sequenceNumber sequence number of the change that has to be removed
     */
    void removeChange(std::uint32_t sequenceNumber);

    /**
     * @brief method for updating the fragment status based on NackFrag information
     *
     * @param msg message containing the nackFrag
     * @return changeStatus enum
     */
    changeStatus processNack(NackFrag *msg);

    /**
     * @brief method for updating the fragment status based on NackFrag information
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if complete, else returns false
     */
    bool checkSampleCompleteness(std::uint32_t sequenceNumber);

    /**
     * @brief Used for resetting fragment states to 'UNSENT' if not acked yet
     *
     * @param sequenceNumber seq number of sample that shall be updated
     * @return true if sample was found and fragments reset, else returns false (indicating sample complete or not found)
     */
    bool resetTimedOutFragments();

    /**
     * @brief set timeout timestamp of reader proxy
     *
     * @param timestamp timestamp for when to trigger timeout
     * @param sequenceNumber sequence number of sample that shall be timed out
     */
    void setTimeout(std::chrono::system_clock::time_point timestamp, std::uint32_t sequenceNumber);

    /**
     * @brief get timeout timestamp of reader proxy
     *
     * @return timestamp of timeout if active, else returns epoch time
     */
    std::chrono::system_clock::time_point getTimeout();

    /**
     * @brief check whether timeout is active and timestamp elapsed
     *
     * @return timeoutStatus enum
     */
    timeoutStatus checkTimeout();

    /**
     * @brief return the number of acknowledged fragments from the
     * respective sample with corresponding sequence number
     *
     * @param sequenceNumber relevant sample sequence number
     */
    std::uint32_t getAckCount(std::uint32_t sequenceNumber);


};

}; // end namespace

#endif // W2RP_READERPROXY_H_
