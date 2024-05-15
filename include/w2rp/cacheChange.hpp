/*
 *
 */

#ifndef W2RP_CACHECHANGE_H_
#define W2RP_CACHECHANGE_H_

#include <math.h>
#include <chrono>
#include <vector>
#include <w2rp/sampleFragment.hpp>



namespace w2rp {

class SampleFragment;

enum fragmentStates{
    UNSENT,
    SENT,
    ACKED,
    NACKED,
    RECEIVED,
    TIMEOUT // added for W2RP
};

class CacheChange
{
  public:
    /// sequence number of current sample
    uint32_t sequenceNumber;
    /// size of the sample in bytes
    uint32_t sampleSize;
    /// size of each fragment
    uint32_t fragmentSize;
    /// total number of fragments per sample
    uint32_t numberFragments;


    /// timestamp: when did the sample arrive at the middleware
    std::chrono::system_clock::time_point arrivalTime;

    /// status flag for indicating complete transmission of a sample (to a reader)
    bool complete;

    /// array storing the 'data'
    SampleFragment** sampleFragmentArray;

    /*
     * empty default constructor
     */
    CacheChange() {};

    /**
     * @brief overloaded constructor
     *
     * @param seqNum sequence number of the current sample
     * @param sampleSize size of the sample in bytes
     * @param fragmentSize size of a fragment in bytes
     * @param timestamp time when sample was generated/arrived at middleware
     */
    CacheChange(uint32_t seqNum, uint32_t sampleSize, uint32_t fragmentSize, std::chrono::system_clock::time_point timestamp):
        sequenceNumber(seqNum),
        sampleSize(sampleSize),
        fragmentSize(fragmentSize),
        numberFragments(int(ceil(sampleSize / fragmentSize))),
        arrivalTime(timestamp)
    {
        // sampleFragmentArray = new SampleFragment*[this->numberFragments];

        // // instantiate all fragments comprising the sample
        // for(uint32_t i = 0; i < this->numberFragments; i++){
        //     sampleFragmentArray[i] = new SampleFragment(this,
        //                                                 i,
        //                                                 (fragmentSize < sampleSize - (i*fragmentSize)) ? fragmentSize : sampleSize - (i*fragmentSize),
        //                                                 arrivalTime);
        // }
    };

    /**
     * @brief copy constructor
     */
    CacheChange(CacheChange &change):
        sequenceNumber(change.sequenceNumber),
        sampleSize(change.sampleSize),
        fragmentSize(change.fragmentSize),
        numberFragments(change.numberFragments),
        arrivalTime(change.arrivalTime)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        auto sampleArrayRef = change.getFragmentArray();

        // copy contents of reference array (CacheChange) to this instance's array
        for(uint32_t i = 0; i < this->numberFragments; i++){
            sampleFragmentArray[i] = new SampleFragment(*sampleArrayRef[i]);
        }
    }

    /**
     * @brief empty default destructor
     */
    ~CacheChange()
    {
        for(int i = 0; i < numberFragments; i++)
        {
            SampleFragment* fragment = this->sampleFragmentArray[i];
            delete fragment;
        }
    };

    /**
     * @brief method for altering a fragment's status (unsent, sent, acked, ...)
     *
     * @param status fragment status
     * @param fragmentNumber fn of fragment to be updated
     * @return true if successful, else false
     */
    bool setFragmentStatus(fragmentStates status, uint32_t fragmentNumber)
    {
        // actual implementations found in ChangeForReader/Writer
        return false;
    };


    /**
     * @brief set the fragment array
     *
     * @param vector of sample fragments 
     */
    void setFragmentArray(std::vector<SampleFragment*> *fragments)
    {
        sampleFragmentArray = new SampleFragment*[this->numberFragments];

        uint16_t i = 0;
        for (auto sf: *fragments)
        {
            sampleFragmentArray[i] = new SampleFragment(*sf);
            i++;
        }        
    }

    /**
     * @brief return the fragment array
     *
     * @return array of sample fragments
     */
    SampleFragment **getFragmentArray()
    {
        return this->sampleFragmentArray;
    }


    /**
     * @brief determine whether age has been exceeded
     *
     * @param deadline sample deadline
     * @return true if sample still valid, else returns false
     */
    bool isValid(std::chrono::system_clock::duration deadline)
    {
        auto now = std::chrono::system_clock::now();
        return ((now - this->arrivalTime) < deadline);
    }

    /**
     * @brief check whether sample has been received or acknowledged in its entirety
     *
     * @return true if complete, else returns false
     */
    bool checkForCompleteness()
    {
       bool tmp = true;
       for(int i = 0; i < this->numberFragments; i++){
           SampleFragment* fragment = this->sampleFragmentArray[i];
           if(!fragment->acked && !fragment->received){
               return false;
           }
       }
       this->complete = tmp;
       return this->complete;
    };

};

}; // end namespace

#endif // W2RP_CACHECHANGE_H_
