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
    std::chrono::system_clock::time_point sentTime;

    /// flag fragment as sent - relevant for writer
    bool sent;
    /// flag fragment as acknowledged - relevant for writer
    bool acked;
    /// flag fragment as received - relevant for reader
    bool received;


  public:
    /**
     * @brief empty default constructor
     */
    SampleFragment():
        sendCounter(0),
        // Relevant for the Writer
        sent(false),
        acked(false),
        // Relevant for the Reader
        received(false),
        baseChange(baseChange)
    {};

    /**
     * @brief overloaded constructor, creates sampleFragment without data
     *
     * @param baseChange reference (pointer) to Change, the fragment is associated to
     * @param fragStartNum fragment number
     * @param dataSize size of the fragment in bytes
     * @param sentTime time when sample first arrived at middleware
     */
    SampleFragment(CacheChange *baseChange, unsigned int fragStartNum, unsigned int dataSize, std::chrono::system_clock::time_point sentTime):
        fragmentStartingNum(fragStartNum),
        sendCounter(0),
        dataSize(dataSize),
        sentTime(sentTime),
        // Relevant for the Writer
        sent(false),
        acked(false),
        // Relevant for the Reader
        received(false),
        baseChange(baseChange)
    {};

    /**
     * @brief copy constructor
     *
     * @param sf reference to object to be copied
     */
    SampleFragment(SampleFragment &sf):
        fragmentStartingNum(sf.fragmentStartingNum),
        sendCounter(sf.sendCounter),
        dataSize(sf.dataSize),
        data(sf.data),
        sentTime(sf.sentTime),
        sent(sf.sent),
        acked(sf.acked),
        received(sf.received),
        baseChange(sf.baseChange)
    {};

    /**
     * @brief empty default destructor
     */
    ~SampleFragment() {};

    /**
     * @brief comparison operator
     */
    bool operator == (const SampleFragment& other) const
    {
        return ((this->dataSize == other.dataSize) &&
               (0 == memcmp(this->data, other.data, this->dataSize)) &&
               (this->fragmentStartingNum == other.fragmentStartingNum));
    }   


    /**
     * @brief set status of flag 'sent'
     *
     * @param b set 'sent' to this boolean value
     */
    void setSent(bool b)
    {
        if(!(this->acked))
        {
            this->sent = b;

            this->sentTime = std::chrono::system_clock::now();;
            sendCounter++;
        }
    };

    /**
     * @brief set status of flag 'acked'
     *
     * @param b set 'acked' to this boolean value
     */
    void setAcked(bool b)
    {
        this->acked = b;
    };

    /**
     * @brief set status of flag 'received'
     *
     * @param b set 'received' to this boolean value
     */
    void setReceived(bool b)
    {
        this->received = b;
    };

    /**
     * @brief set data
     *
     * @param binaryData binary representation of the fragment data 
     */
    void setData(unsigned char* binaryData, uint32_t size, uint32_t fragmentNum, std::chrono::system_clock::time_point sentTime)
    {
        memcpy(this->data, binaryData, size);
        this->dataSize = size;
        this->fragmentStartingNum = fragmentNum;
        this->sentTime = sentTime;
    };

    /**
     * @brief set base change
     *
     * @param baseChange pointer to base change
     */
    void setBaseChange(CacheChange *baseChange)
    {
        this->baseChange = baseChange;
    };
};

}; // end namespace

#endif // W2RP_ENTITIES_SAMPLEFRAGMENT_H_
