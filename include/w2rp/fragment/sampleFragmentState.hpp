#ifndef W2RP_ENTITIES_SAMPLEFRAGMENTSTATUS_H_
#define W2RP_ENTITIES_SAMPLEFRAGMENTSTATUS_H_

#include <chrono>
#include <cstring>
#include <memory>
#include <cstdint>

#include <w2rp/log.hpp>


namespace w2rp {

class SampleFragmentState
{
public:

    enum FragmentStatus{
        UNSENT,
        SENT,
        ACKED,
        NACKED,
        RECEIVED,
        TIMEOUT // added for W2RP
    };

    SampleFragmentState(SampleFragment* fragment):
        number(fragment->number),
        status(UNSENT),
        sendCounter(0)
    {};

    /**
     * @brief set status of flag 'sent'
     *
     * @param b set 'sent' to this boolean value
     */
    void setSent()
    {
        if(status != ACKED)
        {
            status = SENT;
            sentTime = std::chrono::system_clock::now();;
            sendCounter++;
        }
    };

    void setUnsent()
    {
        if(status != ACKED)
        {
            status = UNSENT;
        }
    };

    /**
     * @brief set status to 'acked'
     */
    void setAcked()
    {
        status = ACKED;
    };

    /**
     * @brief set status to 'received'
     */
    void setReceived()
    {
        status = RECEIVED;
    };

    /**
     * @brief set status to 'nacked'
     */
    void setNacked()
    {
        if(status != ACKED && status != UNSENT)
        {
            status = NACKED;
        }
    };

    /**
     * @brief set status to 'timeout'
     */
    void setTimeout()
    {
        status = TIMEOUT;
    };

    /**
     * @brief Get the Sent Time object
     * 
     * @return std::chrono::system_clock::time_point 
     */
    std::chrono::system_clock::time_point getSentTime()
    {
        return sentTime;
    };

    /**
     * @brief Get the Status object
     * 
     * @return FragmentStatus 
     */
    FragmentStatus getStatus()
    {
        return status;
    };

    /**
     * @brief Get the Scheduled For Tx object
     * 
     * @return true 
     * @return false 
     */
    bool isScheduledForTx()
    {
        return scheduledForTx;
    };

    /**
     * @brief Set the Scheduled For Tx object
     * 
     * @param scheduled 
     */
    void setScheduledForTx(bool scheduled)
    {
        scheduledForTx = scheduled;
    };

private:
    // Pointer to associated sample fragment
    std::uint32_t number;

    // Status is needed for the writer to determine selection for Tx, ReTx
    FragmentStatus status;
    
    // timestamp the fragment has been sent (the latest timestamp if already sent multiple times)
    std::chrono::system_clock::time_point sentTime;

    // counting how often the given fragment has been transmitted
    std::uint32_t sendCounter;

    // flag indicating whether the fragment is scheduled for transmission
    bool scheduledForTx = false;

};

}; // end namespace w2rp

#endif // W2RP_ENTITIES_SAMPLEFRAGMENTSTATUS_H_
