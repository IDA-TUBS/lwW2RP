/*
 *
 */

#ifndef W2RP_WRITERPROXY_H_
#define W2RP_WRITERPROXY_H_

#include <math.h>
#include <list>
#include <w2rp/changeForWriter.h>

namespace w2rp {

class SampleFragment;

class WriterProxy
{
  private:
    /// history story all samples
    std::list<ChangeForWriter*> history;
    /// latest sample number;
    int highestSequenceNumber;

    /// max size of history
    uint32_t historySize;

  public:
    /*
     * default constructor
     */
    WriterProxy(uint32_t historySize):
        historySize(historySize),
        highestSequenceNumber(-1)
    {};

    /*
     * overloaded constructor, add first change to history
     */
    WriterProxy(uint32_t historySize, CacheChange &change):
        historySize(historySize)
    {
        this->addChange(change);
    };

    /*
     * default destructor
     */
    ~WriterProxy()
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
     * method for removing a Cache Change from the proxie's history cache
     *
     * @param sequenceNumber sequence number of the change that has be be removed
     */
    void removeChange(uint32_t sequenceNumber);

    /*
     * method for altering a fragment's status (unsent, sent, acked, ...)
     *
     * @param status fragment status
     * @param sequenceNumber seq number of the sample which fragment status shall be altered
     * @param fragmentNumber fn of fragment to be updated
     * @return true if successful, else false
     */
    bool updateFragmentStatus (fragmentStates status, uint32_t sequenceNumber, uint32_t fragmentNumber);

    /*
     * method for evaluating completeness of samples
     *
     * @param sequenceNumber seq number of sample that shall be checked
     * @return true if complete, else returns false
     */
    bool checkSampleCompleteness(uint32_t sequenceNumber);

    /*
     * method returning the current (oldest) cache change
     *
     * @return ChangeForWriter object
     */
    ChangeForWriter* getCurrentChange()
    {
        return history.front();
    };

    /*
     * method returning the latest cache change
     *
     * @return ChangeForWriter object
     */
    ChangeForWriter* getLatestChange()
    {
        return history.back();
    };

    /*
     * Method for returning history size
     */
    uint32_t getSize()
    {
        return history.size();
    };

    /*
     * method cache change with corresponding sequence number
     *
     * @param sequenceNumber seq number of sample that shall be retrieved
     * @return ChangeForWriter object
     */
    ChangeForWriter* getChange(uint32_t sequenceNumber)
    {
        // first find change corresponding to the given sequence number
        ChangeForWriter* tmp = nullptr;
        for (auto cfw: history)
        {
            if (cfw->sequenceNumber == sequenceNumber)
            {
                tmp = cfw;
                break;
            }
        }

        return tmp;
    };
};

}; // end namespace

#endif // W2RP_WRITERPROXY_H_
