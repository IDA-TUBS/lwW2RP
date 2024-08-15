/*
 *
 */

#ifndef W2RP_READERPROXY_H_
#define W2RP_READERPROXY_H_

#include <math.h>
#include <vector>
#include <list>
#include <w2rp/changeForReader.hpp>
#include <w2rp/messages/messages.hpp>

namespace w2rp {

class SampleFragment;

class ReaderProxy
{
  private:
    uint32_t mem_guard = 0;
    /// history story all samples
    std::list<ChangeForReader*> history;
    /// max size of history
    uint32_t historySize;

    /// enabling NACK suppresion
    bool nackSuppressionEnabled;
    /// NACK suppression duration
    std::chrono::system_clock::duration nackSuppressionDuration;

  public:
    /// entity of corresponding reader -> Simplification: host ID
    uint32_t readerID;  
    /// the reader's priority
    uint32_t priority;

    /// flag signaling a timeout is active - used for WiMEP
    bool timeoutActive;
    /// timestamp of next TO if active
    std::chrono::system_clock::time_point timeoutTS;

    std::mutex rp_mutex;

    /**
     * @brief default constructor
     */
    ReaderProxy(uint32_t id, uint32_t historySize):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(false),
        timeoutActive(false),
        history()
    {
    };

    /**
     * @brief overloaded constructor, enable NACK suppression
     */
    ReaderProxy(uint32_t id, uint32_t historySize, std::chrono::system_clock::duration nackSuppressionDuration):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(true),
        nackSuppressionDuration(nackSuppressionDuration),
        timeoutActive(false),
        history()
    {
    };

    /**
     * @brief overloaded constructor, add first change to history
     */
    ReaderProxy(uint32_t id, uint32_t historySize, CacheChange &change):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(false),
        timeoutActive(false)
    {
        std::unique_lock<std::mutex> lock(rp_mutex);
        this->addChange(change);
        lock.unlock();
    };

    /**
     * @brief overloaded constructor, enable NACK suppression and add first change to history
     */
    ReaderProxy(uint32_t id, uint32_t historySize, std::chrono::system_clock::duration nackSuppressionDuration, CacheChange &change):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(true),
        nackSuppressionDuration(nackSuppressionDuration),
        timeoutActive(false)
    {
        std::unique_lock<std::mutex> lock(rp_mutex);
        this->addChange(change);
        lock.unlock();
    };

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
     * @brief method for adding a new Cache Change to the proxy's history cache
     *
     * @param change reference to cache change that will be replicated in the reader proxy
     * @return true if change was added, false if cache was full and change was not added to history
     */
    bool addChange(CacheChange &change);

    /**
     * @brief get readerID (entity ID) of corresponding reader, added for WiMEP protocol
     *
     * @return entity id
     */
    uint32_t getReaderId()
    {
        return readerID;
    }

    /**
     * @brief set priority of corresponding reader, added for WiMEP protocol
     *
     * @param prio the priority assigned to the corresponding reader
     */
    void setPriority(uint32_t prio)
    {
        this->priority = prio;
    }

    /**
     * @brief method for removing a Cache Change from the proxie's history cache
     *
     * @param sequenceNumber sequence number of the change that has to be removed
     */
    void removeChange(uint32_t sequenceNumber);

    /**
     * @brief method for checking whether a Cache Change is in the history cache
     *
     * @param sequenceNumber sequence number of the change that has be be removed
     * @return bool
     */
    bool changeExists(uint32_t sequenceNumber);

    /**
     * @brief method for altering a fragment's status (unsent, sent, acked, ...)
     *
     * @param status fragment status
     * @param sequenceNumber seq number of the sample which fragment status shall be altered
     * @param fragmentNumber fn of fragment to be updated
     * @return true if successful, else false
     */
    bool updateFragmentStatus (fragmentStates status, uint32_t sequenceNumber, uint32_t fragmentNumber, std::chrono::system_clock::time_point sentTimestamp);

    /**
     * @brief method for updating the fragment status based on NackFrag information
     *
     * @param nackFrag message containing the nackFrag
     * @return true if successful, else returns false
     */
    bool processNack(NackFrag *msg);

    /**
     * @brief method for updating the fragment status based on NackFrag information
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if complete, else returns false
     */
    bool checkSampleCompleteness(uint32_t sequenceNumber);

    /**
     * @brief method returning the current (oldest) cache change
     */
    ChangeForReader* getCurrentChange()
    {
        std::unique_lock<std::mutex> lock(rp_mutex);
        return history.front();
        lock.unlock();
    }

    /**
     * @brief gather all fragments of a given change that are currently in state unsent (and not acknowledged!)
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return list of fragments
     */
    std::vector<SampleFragment*> getUnsentFragments(uint32_t sequenceNumber);

    /**
     * @brief determine whether a timeout is needed to ensure safe and complete sample transmission
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if a timeout needs to be triggered, else returns false
     */
    bool checkForTimeout(uint32_t sequenceNumber);

    /**
     * @brief WiMEP function, used for resetting fragment states to 'UNSENT' if not acked yet
     *
     * @param sequenceNumber seq number of sample that shall be updated
     */
    void resetTimeoutedFragments(uint32_t sequenceNumber);


    /**
     * @brief set timeout timestamp of reader proxy
     *
     * @param timestamp timestamp for when to trigger timeout
     */
    void setTimeoutTimestamp(std::chrono::system_clock::time_point timestamp)
    {
        this->timeoutActive = true;
        this->timeoutTS = timestamp;
    }

    /**
     * @brief set timeout timestamp of reader proxy
     *
     * @param timestamp timestamp for when to trigger timeout
     */
    std::chrono::system_clock::time_point getTimeoutTimestamp()
    {
        return this->timeoutTS;
    }

    /**
     * @brief return the number of acknowledged fragments from the
     * respective sample with corresponding sequence number
     *
     * @param sequenceNumber relevant sample sequence number
     */
    uint32_t getAckCount(uint32_t sequenceNumber);


};

}; // end namespace

#endif // W2RP_READERPROXY_H_
