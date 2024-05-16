/*
 *
 */

#ifndef W2RP_CHANGEFORREADER_H_
#define W2RP_CHANGEFORREADER_H_

#include <math.h>
#include <vector>
#include <chrono>
#include <w2rp/cacheChange.hpp>
#include <w2rp/sampleFragment.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

class SampleFragment;

class ChangeForReader: public CacheChange
{
  private:
    /// reader entity ID
    uint32_t readerID;

  public:
    /// flag for signaling that all fragments have been acknowledged
    bool complete;

    /// storing fragment number that has been sent last
    int lastSentFN;
    /// highest yet sent fragment number
    int highestFNSend;

    /// reference (pointer) to intial CacheChange
    CacheChange *baseChange;


    /**
     * @brief empty default constructor
     */
    ChangeForReader(){
        complete = false;
        lastSentFN = -1;
        highestFNSend = 0;
    }

    /**
     * @brief overloaded constructor
     *
     * @param id of reader
     * @param seqNum sequence number of the current sample
     * @param sampleSize size of the sample in bytes
     * @param fragmentSize size of a fragment in bytes
     * @param timestamp time when sample was generated/arrived at middleware
     */
    ChangeForReader(CacheChange* change, uint32_t id, uint32_t seqNum, uint32_t sampleSize, uint32_t fragmentSize, std::chrono::system_clock::time_point timestamp):
        CacheChange(seqNum, sampleSize, fragmentSize, timestamp),
        readerID(id),
        complete(false),
        lastSentFN(-1),
        highestFNSend(0),
        baseChange(change)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        // instantiate all fragments comprising the sample
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(baseChange,
                                                        i,
                                                        (fragmentSize < sampleSize - (i*fragmentSize)) ? fragmentSize : sampleSize - (i*fragmentSize),
                                                        arrivalTime);
        }
    };

    /**
     * @brief overloaded constructor, generate ChangeForReader from CacheChange
     *
     * @param id of reader
     * @param change reference to cache change
     */
    ChangeForReader(uint32_t id, CacheChange &change):
        CacheChange(change.sequenceNumber, change.sampleSize, change.fragmentSize, change.arrivalTime),
        readerID(id),
        complete(false),
        lastSentFN(-1),
        highestFNSend(0),
        baseChange(&change)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        auto sampleArrayRef = change.getFragmentArray();
        // copy contents of reference array (CacheChange) to this instance's array
        for(uint32_t i = 0; i < this->numberFragments; i++){
            logInfo("[ChangeForReader] create sf " << i)
            sampleFragmentArray[i] = new SampleFragment(*sampleArrayRef[i]);
        }
    };

    /**
     * @brief copy constructor
     */
    ChangeForReader(ChangeForReader &change):
        CacheChange(change.sequenceNumber, change.sampleSize, change.fragmentSize, change.arrivalTime),
        readerID(change.readerID),
        complete(change.complete),
        lastSentFN(change.lastSentFN),
        highestFNSend(change.highestFNSend),
        baseChange(change.baseChange)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        auto sampleArrayRef = change.getFragmentArray();

        // copy contents of reference array to this instance's array
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(*sampleArrayRef[i]);
        }
    }

    /**
     * @brief empty default destructor
     */
    ~ChangeForReader()
    {
        logInfo("[ChangeForReader] delete")
        // delete[] sampleFragmentArray; // Leads to conflicts at object destruction
    };

    /**
     * @brief determine number of fragments that have not been acknowledged yet
     *
     * @return number of fragments
     */
    uint32_t notAckCount();

    /**
     * @brief determine number of fragments that have been acknowledged so far
     *
     * @return number of fragments
     */
    uint32_t ackCount();

    /**
     * @brief determine number of fragments that have been sent so far, also includes those already acknowledged by the reader
     *
     * @return number of fragments
     */
    uint32_t sentCount();

    /**
     * @brief determine number of fragments that are in state "UNSENT"
     *
     * @return number of fragments
     */
    uint32_t unsentCount();

    /**
     * @brief method for updating the fragment status
     *
     * @param statues new fragment status
     * @param fragmentNumber fragment whose fragment shall be updated
     * @return true if operation successful, else returns false
     */
    bool setFragmentStatus(fragmentStates status, uint32_t fragmentNumber, std::chrono::system_clock::time_point sentTimestamp);

    /**
     * @brief gather all fragments of a given change that are currently in state unsent (and not acknowledged!)
     *
     * @return list of fragments
     */
    std::vector<SampleFragment*> getUnsentFragments();

    /**
     * @brief reset all sent but unacknowledged fragments to state 'UNSENT', used by WiMEP
     */
    void resetSentFragments();

};

}; // end namespace

#endif // W2RP_CHANGEFORREADER_H_
