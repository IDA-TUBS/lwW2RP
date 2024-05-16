
#ifndef W2RP_WRITER_H_
#define W2RP_WRITER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <w2rp/readerProxy.hpp>
#include <w2rp/helper/fragmentation.hpp>
#include <w2rp/messages/messages.hpp>
#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/periodicEvent.hpp>
#include <w2rp/timer/timerManager.hpp>

namespace w2rp {

typedef enum
{
    NONE,
    FIXED,
    ADAPTIVE_LOW_PDR,
    ADAPTIVE_HIGH_PDR
} PrioritizationMode;

struct writerCfg
{
    uint32_t fragmentSize;
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration shapingTime;
    std::chrono::system_clock::duration nackSuppressionDuration;
    uint8_t numberReaders;
    std::vector<std::string> readerAddresses;
    unsigned int sizeCache;
    uint8_t writerUuid;
    unsigned char guidPrefix[12];
    PrioritizationMode prioMode;
};


class Writer
{
private:
    /// writer configuration
    writerCfg config;

    /// sample fragmenter
    Fragmentation *sampleFragmenter;

    /**********************************/
    /* protocol management structures */
    /**********************************/

    /// storing the "actual" data that is transmitted via RTPS
    std::list<CacheChange*> historyCache;
    /// the reader proxies keep track of sending/acknowledgment states for each reader
    std::vector<ReaderProxy*> matchedReaders;
    /// list of fragments to send next
    std::list<SampleFragment*> sendQueue;

    /****************/
    /* Timer events */
    /****************/

    TimerManager timer_manager;
    /// Timed Event for resetting fragments
    // TimedEvent *timeoutTimer;
    /// Timed Event for periodic shaping and sample transmissions
    PeriodicEvent<> *shapingTimer;

    /// counter for next sample's sequence number
    uint32_t sequenceNumberCnt;

    /******************/
    /* Message parser */
    /******************/

    NetMessageParser *netParser;


    /******************************/
    /************ misc. ***********/
    /******************************/
    // variable storing the sequence number of the sample that is currently being transmitted
    int currentSampleNumber;
    /// counter for total number of fragments sent
    uint32_t fragmentCounter;
    /// HB counter
    uint32_t hbCounter; 

public:
    /**
     * @brief empty default constructor
     */ 
    Writer();

    /**
     * @brief empty default destructor
     */
    ~Writer();

    /**
     * @brief API to application, receive new samples from app 
     * 
     * @param data (in serialized form)
     */
    bool write(SerializedPayload* data);

protected:

    /********************************************/
    /** Callbacks triggered by external events **/ 
    /********************************************/

    /**
     * @brief Callback 
     *
     * @param net MessageNet_t object containing W2RP submessages
     * @return true: success, false: failure
     */
    bool handleMessages(MessageNet_t *net);


    /**
     * @brief Callback for creating new cache change on arrival of new sample from application
     *
     * @param data (in serialized form) of the sample received from the application
     */
    bool handleNewSample(SerializedPayload* data);

    /**
     * @brief Method for reacting to NackFrags received from readers
     *
     * @param msg message containing the nack bitmap
     */
    void handleNackFrag(NackFrag *msg);


    /**
     * @brief add new sample to cache
     *
     * @param data (in serialized form) of the sample received from the application
     * @param timestamp a sample's arrival timestamp
     */
    bool addSampleToCache(SerializedPayload *data,  std::chrono::system_clock::time_point timestamp);






    /*********************************************/
    /* methods used during fragment transmission */
    /*********************************************/

    /**
     * @brief callback that is triggered according to some schedule. At the end,
     *   sends a packaged sample fragment down towards the UDP/IP stack
     *
     * @return true: success, false: failure
     */
    bool sendMessage();

    /**
     * @brief Method for selecting a reader for the next transmission
     *
     * @return reader proxy
     */
    ReaderProxy* selectReader();

    /**
     * @brief Method for selecting which fragment (missing at a specific reader) to transmit next
     *
     * @param rp pointer to reader proxy used for fragment selection
     * @return the sample fragment to be transmitted next
     */
    SampleFragment* selectNextFragment(ReaderProxy *rp);

    /**
     * @brief Method for priming the send queue with each fragment that needs to be transmitted.
     * Used in WiMEP's transmissions phase and ensure that no retransmissions will be
     * performed prior to each fragment being transmitted once. Always called if a new
     * sample has to be transmitted
     *
     * @param sequenceNumber of the corresponding sample that shall be transmitted next
     */
    void fillSendQueueWithSample(uint32_t sequenceNumber);

    /**
     * @brief create DataFrag submessage
     * 
     * @param sf pointer to sample fragment that shall be transmitted
     * @param ret pointer to data frag submessage  
     */

    void createDataFrag(SampleFragment* sf, DataFrag* ret);


    /**
     * @brief create HeartbeatFrag submessage
     * 
     * @param sf pointer to latest sample fragment
     * @param ret pointer to HeartbeatFrag submessage  
     */

    void createHBFrag(SampleFragment* sf, HeartbeatFrag* ret);




    /*************************************************/
    /* methods for checking validity of cacheChanges */ 
    /*************************************************/
 
    /**
     * @brief Method for evaluating whether a sample is still valid or whether its deadline elapsed.
     * Handles removing of sample in case of elapsed deadline
     */
    void checkSampleLiveliness();
    

    /**
     * @brief Method for removing samples from the history cache in case the sample has been successfully
     * transmitted to ALL matched readers
     */
    void removeCompleteSamples();










    /*****************************/
    /* timeout related functions */ 
    /*****************************/

    /*
     * @brief callback for handling of timeouts
     */
    void handleTimeout();






    


    /***************************/
    /* miscellaneous functions */ 
    /***************************/

};


} //end namespace

#endif //W2RP_WRITER_H_