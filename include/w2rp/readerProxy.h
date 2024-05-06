/*
 *
 */

#ifndef W2RP_READERPROXY_H_
#define W2RP_READERPROXY_H_

#include <math.h>
#include <vector>
#include <list>
#include <w2rp/changeForReader.h>

namespace w2rp {

class SampleFragment;

class ReaderProxy
{
  private:
    /// history story all samples
    std::list<ChangeForReader*> history;
    /// max size of history
    uint32_t historySize;

    /// enabling NACK suppresion
    bool nackSuppressionEnabled;
    /// NACK suppression duration
    std::chrono::system_clock::time_point nackSuppressionDuration;

  public:
    /// entity of corresponding reader
    uint32_t readerID;
    /// the reader's priority
    uint32_t priority;

    /// flag signaling a timeout is active - used for WiMEP
    bool timeoutActive;

    /*
     * default constructor
     */
    ReaderProxy(uint32_t id, uint32_t historySize):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(false),
        timeoutActive(false)
    {};

    /*
     * overloaded constructor, enable NACK suppression
     */
    ReaderProxy(uint32_t id, uint32_t historySize, std::chrono::system_clock::duration nackSuppressionDuration):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(true),
        nackSuppressionDuration(nackSuppressionDuration),
        timeoutActive(false)
    {};

    /*
     * overloaded constructor, add first change to history
     */
    ReaderProxy(uint32_t id, uint32_t historySize, CacheChange &change):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(false),
        timeoutActive(false)
    {
        this->addChange(change);
    };

    /*
     * overloaded constructor, enable NACK suppression and add first change to history
     */
    ReaderProxy(uint32_t id, uint32_t historySize, std::chrono::system_clock::duration nackSuppressionDuration, CacheChange &change):
        readerID(id),
        historySize(historySize),
        nackSuppressionEnabled(true),
        nackSuppressionDuration(nackSuppressionDuration),
        timeoutActive(false)
    {
        this->addChange(change);
    };

    /*
     * default destructor
     */
    ~ReaderProxy()
    {
        history.clear();
    };

    /*
     * method for adding a new Cache Change to the proxy's history cache
     *
     * @param change reference to cache change that will be replicated in the reader proxy
     * @return true if change was added, false if cache was full and change was not added to history
     */
    bool addChange(CacheChange &change);

    /*
     * get readerID (entity ID) of corresponding reader, added for WiMEP protocol
     *
     * @return entity id
     */
    uint32_t getReaderId()
    {
        return readerID;
    }

    /*
     * set priority of corresponding reader, added for WiMEP protocol
     *
     * @param prio the priority assigned to the corresponding reader
     */
    void setPriority(uint32_t prio)
    {
        this->priority = prio;
    }

    /*
     * method for removing a Cache Change from the proxie's history cache
     *
     * @param sequenceNumber sequence number of the change that has to be removed
     */
    void removeChange(uint32_t sequenceNumber);

    /*
     * method for checking whether a Cache Change is in the history cache
     *
     * @param sequenceNumber sequence number of the change that has be be removed
     * @return bool
     */
    bool changeExists(uint32_t sequenceNumber);

    /*
     * method for altering a fragment's status (unsent, sent, acked, ...)
     *
     * @param status fragment status
     * @param sequenceNumber seq number of the sample which fragment status shall be altered
     * @param fragmentNumber fn of fragment to be updated
     * @return true if successful, else false
     */
    bool updateFragmentStatus (fragmentStates status, uint32_t sequenceNumber, uint32_t fragmentNumber, std::chrono::system_clock::time_point sentTimestamp);

    /*
     * method for updating the fragment status based on NackFrag information
     *
     * @param nackFrag message containing the nackFrag
     * @return true if successful, else returns false
     */
    bool processNack(void* nackFrag); // TODO replace with message or similar!

    /*
     * method for updating the fragment status based on NackFrag information
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if complete, else returns false
     */
    bool checkSampleCompleteness(uint32_t sequenceNumber);

    /*
     * method returning the current (oldest) cache change
     */
    ChangeForReader* getCurrentChange()
    {
        return history.front();
    }

    /*
     * gather all fragments of a given change that are currently in state unsent (and not acknowledged!)
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return list of fragments
     */
    std::vector<SampleFragment*> getUnsentFragments(uint32_t sequenceNumber);

    /*
     * determine whether a timeout is needed to ensure safe and complete sample transmission
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if a timeout needs to be triggered, else returns false
     */
    bool checkForTimeout(uint32_t sequenceNumber);

    /*
     * WiMEP function, used for resetting fragment states to 'UNSENT' if not acked yet
     *
     * @param sequenceNumber seq number of sample that shall be updated
     */
    void resetTimeoutedFragments(uint32_t sequenceNumber);

};

}; // end namespace

#endif // W2RP_READERPROXY_H_
