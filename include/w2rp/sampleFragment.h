/*
 *
 */

#ifndef W2RP_ENTITIES_SAMPLEFRAGMENT_H_
#define W2RP_ENTITIES_SAMPLEFRAGMENT_H_

#include <chrono>
#include <cstring>

namespace w2rp {

class CacheChange;

class SampleFragment
{
  public:
    /// fragment number
    uint32_t fragmentStartingNum;
    /// counting how often the given fragment has been transmitted
    uint32_t sendCounter;
    /// actual fragment size (can be smaller than fragmentSize)
    uint32_t dataSize;

    /// reference (pointer) to ChangeForReader owning this object
    CacheChange* baseChange;

    /// actual data
    unsigned char* data;

    /// timestamp the fragment has been sent (the latest timestamp if already sent multiple times)
    std::chrono::system_clock::time_point sendTime;

    /// flag fragment as sent - relevant for writer
    bool sent;
    /// flag fragment as acknowledged - relevant for writer
    bool acked;
    /// flag fragment as received - relevant for reader
    bool received;


  public:
    /*
     * empty default constructor
     */
    SampleFragment() {};

    /*
     * overloaded constructor
     *
     * @param baseChange reference (pointer) to Change, the fragment is associated to
     * @param fragStartNum fragment number
     * @param dataSize size of the fragment in bytes
     * @param sendTime time when sample first arrived at middleware
     */
    SampleFragment(CacheChange *baseChange, uint32_t fragStartNum, uint32_t dataSize, std::chrono::system_clock::time_point sendTime):
        fragmentStartingNum(fragStartNum),
        sendCounter(0),
        dataSize(dataSize),
        sendTime(sendTime),
        // Relevant for the Writer
        sent(false),
        acked(false),
        // Relevant for the Reader
        received(false),
        baseChange(baseChange)
    {};

    /*
     * copy constructor
     *
     * @param sf reference to object to be copied
     */
    SampleFragment(SampleFragment &sf):
        fragmentStartingNum(sf.fragmentStartingNum),
        sendCounter(sf.sendCounter),
        dataSize(sf.dataSize),
        sendTime(sf.sendTime),
        sent(sf.sent),
        acked(sf.acked),
        received(sf.received),
        baseChange(sf.baseChange)
    {};

    /*
     * empty default destructor
     */
    ~SampleFragment() {};

    /*
     * comparison operator
     */
    bool operator == (const SampleFragment& other) const
    {
        return ((this->dataSize == other.dataSize) &&
               (0 == memcmp(this->data, other.data, this->dataSize)) &&
               (this->fragmentStartingNum == other.fragmentStartingNum));
    }   


    /*
     * set status of flag 'sent'
     *
     * @param b set 'sent' to this boolean value
     */
    void setSent(bool b)
    {
        if(!(this->acked))
        {
            this->sent = b;

            this->sendTime = std::chrono::system_clock::now();;
            sendCounter++;
        }
    };

    /*
     * set status of flag 'acked'
     *
     * @param b set 'acked' to this boolean value
     */
    void setAcked(bool b)
    {
        this->acked = b;
    };

    /*
     * set status of flag 'received'
     *
     * @param b set 'received' to this boolean value
     */
    void setReceived(bool b)
    {
        this->received = b;
    };

    /*
     * set data
     *
     * @param binaryData binary representation of the fragment data 
     */
    void setData(unsigned char* binaryData, uint32_t size)
    {
        this->data = binaryData;
        this->dataSize = size;
    }
};

}; // end namespace

#endif // W2RP_ENTITIES_SAMPLEFRAGMENT_H_
