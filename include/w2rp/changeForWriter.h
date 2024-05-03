/*
 *
 */

#ifndef W2RP_CHANGEFORWRITER_H_
#define W2RP_CHANGEFORWRITER_H_

#include <math.h>
#include <chrono>
#include <w2rp/cacheChange.h>
#include <w2rp/sampleFragment.h>

namespace w2rp {

class SampleFragment;

class ChangeForWriter: public CacheChange
{
  private:
    /// flag for signaling that all fragments have been acknowledged
    bool complete;

    /// storing fragment number that has been sent last
    int lastReceivedFN;
    /// highest yet sent fragment number
    int highestFNreceived;

  public:
    /*
     * empty default constructor
     */
    ChangeForWriter()
    {
        complete = false;
        lastReceivedFN = -1;
        highestFNreceived = 0;
    }

    /*
     * overloaded constructor
     *
     * @param id of reader
     * @param seqNum sequence number of the current sample
     * @param sampleSize size of the sample in bytes
     * @param fragmentSize size of a fragment in bytes
     * @param timestamp time when sample was generated/arrived at middleware
     */
    ChangeForWriter(uint32_t seqNum, uint32_t sampleSize, uint32_t fragmentSize, std::chrono::system_clock::time_point timestamp):
        CacheChange(seqNum, sampleSize, fragmentSize, timestamp),
        complete(false),
        lastReceivedFN(-1),
        highestFNreceived(0)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        // instantiate all fragments comprising the sample
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(this,
                                                        i,
                                                        (fragmentSize < sampleSize - (i*fragmentSize)) ? fragmentSize : sampleSize - (i*fragmentSize),
                                                        arrivalTime);
        }
    };

    /*
     * overloaded constructor
     *
     * @param CacheChange object
     */
    ChangeForWriter(CacheChange &change):
        CacheChange(change.sequenceNumber, change.sampleSize, change.fragmentSize, change.arrivalTime),
        complete(false),
        lastReceivedFN(-1),
        highestFNreceived(0)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        // instantiate all fragments comprising the sample
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(this,
                                                        i,
                                                        (fragmentSize < sampleSize - (i*fragmentSize)) ? fragmentSize : sampleSize - (i*fragmentSize),
                                                        arrivalTime);
        }
    };

    /*
     * copy constructor
     * *
     * @param ChangeForWriter object
     */
    ChangeForWriter(ChangeForWriter &change):
        CacheChange(change.sequenceNumber, change.sampleSize, change.fragmentSize, change.arrivalTime),
        complete(change.complete),
        lastReceivedFN(change.lastReceivedFN),
        highestFNreceived(change.highestFNreceived)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];
        auto sampleArrayRef = change.getFragmentArray();

        // copy contents of reference array to this instance's array
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(*sampleArrayRef[i]);
        }
    }

    /*
     * empty default destructor
     */
    ~ChangeForWriter()
    {
//        delete[] sampleFragmentArray; // Leads to conflicts at object destruction
    };

    /*
     * determine number of fragments that have been received so far
     *
     * @return number of fragments
     */
    uint32_t receivedCount();

    /*
     * method for updating the fragment status
     *
     * @param statues new fragment status
     * @param fragmentNumber fragment whose fragment shall be updated
     * @return true if operation successful, else returns false
     */
    virtual bool setFragmentStatus(fragmentStates status, uint32_t fragmentNumber);
};

}; // end namespace

#endif // W2RP_CHANGEFORWRITER_H_
