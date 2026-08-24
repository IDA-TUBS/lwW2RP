
#ifndef W2RP_WRITER_H_
#define W2RP_WRITER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <cstdint>
#include <variant>

#include <w2rp/readerProxy.hpp>
#include <w2rp/qos/qos.hpp>
#include <w2rp/helper/safe_queue.hpp>
#include <w2rp/helper/circularBuffer.hpp>
#include <w2rp/messages/messages.hpp>
#include <w2rp/messages/messageUtils.hpp>
#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/periodicEvent.hpp>
#include <w2rp/timer/timerManager.hpp>
#include <w2rp/comm/UDPComm.hpp>
#include <w2rp/config/writerConfig.hpp>
#include <w2rp/guid/guid.hpp>
#include <w2rp/guid/guidPrefix.hpp>
#include <w2rp/guid/guidPrefixManager.hpp>



namespace w2rp {

using SerializedPayload = std::vector<unsigned char>;

class Writer
{
private:

    /* -------------------- Identification and Configuration -------------------- */

    // writer configuration
    config::writerCfg config;

    // rtps guid
    GUID_t guid;

    /* --------------------- protocol management structures --------------------- */

    // Sensor sample input queue
    SafeQueue<SerializedPayload> sampleQueue;

    // storing the "actual" data that is transmitted via RTPS
    stableCircularBuffer<CacheChange> historyCache; 
    
    // the reader proxy keeps track of sending/acknowledgment states for each reader
    ReaderProxy matchedReader;
    
    // mutex for thread synchronization 
    std::mutex history_mutex; 

    /* ------------------------------ Timer events ------------------------------ */

    // Entity for managingtimer events
    TimerManager timer_manager;
    
    // Timed Event for resetting fragments
    TimedEvent<> timeoutTimer;
    
    // Timed Event for periodic shaping and sample transmissions
    PeriodicEvent<> shapingTimer;

    // counter for next sample's sequence number
    std::uint32_t sequenceNumberCnt;

    /* --------------------- Communication and Message Types -------------------- */
    
    // UDP Comm abstraction
    UDPComm CommInterface;

    // HeartbeatFrag for History Cache Synchronization
    HeartbeatFrag hbFrag;

    // feedback channel receive queue
    SafeQueue<MessageNet_t> receiveQueue;

    // std::visit processor for incoming messages
    struct NackProcessor
    {
        Writer* writerInstance;
        W2RPHeader* header;

        // Constructor to set the context
        NackProcessor(Writer* writer, W2RPHeader* hdr) : 
            writerInstance(writer), header(hdr) {}

        void operator()(NackFrag& nackFrag) const 
        {
            writerInstance->handleNackFrag(this->header, &nackFrag);
        }

        void operator()(HeartbeatFrag& hbFrag) const 
        {
            logDebug("[Writer][MessageProcessor] Received HeartbeatFrag - ignoring")
        }

        void operator()(DataFrag& dataFrag) const 
        {
            logDebug("[Writer][MessageProcessor] Received DataFrag - ignoring")
        }
    };


    /* ---------------------------- Timeout handling ---------------------------- */

    std::queue<ReaderProxy*> timeoutQueue;
  
    /* ---------------------------------- misc ---------------------------------- */

    // variable storing the sequence number of the sample that is currently being transmitted
    std::uint32_t currentSampleNumber;

    // variable storing the arrival timestamp of the sample that is currently being transmitted
    std::chrono::system_clock::time_point currentSampleArrival;

    // variable storing the change that is currently being transmitted
    CacheChange* currentChange;

    // variable storing the changeForReader of the sample that is currently being transmitted
    ChangeForReader* currentChangeForReader;
    
    // counter for total number of fragments sent
    std::uint32_t fragmentCounter;
    
    // HB counter
    std::uint32_t hbCounter; 

    /* --------------------------------- Threads -------------------------------- */
    
    // Writer logic thread for sample fragmentation and processing
    std::thread writerThread;

    // socket receive thread
    std::thread recvThread;

    // receive handler thread
    std::thread handlerThread; 


public:
    /**
     * @brief Construct a new Writer object
     * 
     * @param participant_id
     * @param cfg 
     */
    Writer(std::uint16_t participant_id, config::writerCfg cfg);

    /**
     * @brief empty default destructor
     */
    ~Writer();

    /**
     * @brief API to application, receive new samples from app 
     * 
     * @param data (in serialized form)
     */
    void write(SerializedPayload& data);

    /**
     * @brief get callback function for updating the writer's endpoint
     * 
     * @return std::function<void(std::string, int)> 
     */
    std::function<void(std::string, int)> get_callback()
    {
        return std::bind(&Writer::updateTxEndpoint, this, std::placeholders::_1, std::placeholders::_2);
    }

protected:
    /**
     * @brief main writer thread for processing incoming samples and sending fragments
     */
    void sampleProcessing();

    /* ----------------- Callbacks triggered by external events ----------------- */

    /**
     * @brief Blocking reception of incoming message at socket, put into receiveQueue
     */
    void receiveMessage();

    /**
     * @brief Processing of messages from receiveQueue 
     */
    void handleMessage();

    /**
     * @brief Callback for creating new cache change on arrival of new sample from application
     *
     * @param data (in serialized form) of the sample received from the application
     */
    bool handleNewSample(SerializedPayload& data);

    /**
     * @brief Method for reacting to NackFrags received from readers
     *
     * @param header  message header containing identification data
     * @param msg message containing the nack bitmap
     */
    void handleNackFrag(W2RPHeader *header, NackFrag *msg);

    /**
     * @brief add new sample to cache
     *
     * @param data (in serialized form) of the sample received from the application
     * @param timestamp a sample's arrival timestamp
     */
    bool addSampleToCache(SerializedPayload& data,  std::chrono::system_clock::time_point timestamp);


    /* ---------------- methods used during fragment transmission --------------- */

    /**
     * @brief callback that is triggered according to some schedule. At the end,
     *   sends a packaged sample fragment down towards the UDP/IP stack
     *
     * @return true: success, false: failure
     */
    bool sendMessage();

    /* -------------- methods for checking validity of cacheChanges ------------- */
 
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

    /**
     * @brief Method for removing a specific sample from the history cache in case the sample has been successfully
     * transmitted to the matched reader
     * 
     */
    void removeCurrentSample();

    /**
     * @brief Method for checking whether the current sample can be completely transmitted
     *        within its deadline considering the shaping parameters configured
     * 
     * @return true if transmission possible within deadline, false otherwise
     */
    bool isDeadlineAchievable(uint64_t& frag_num);


    /* ------------------------ timeout related functions ----------------------- */

    /**
     * @brief callback for handling of timeouts
     */
    void handleTimeout();

    /* -------------------------- RM related functions -------------------------- */

    /**
     * @brief callback for handling of timeouts
     */
    void updateTxEndpoint(std::string endpoint_tx, int port);

};


} //end namespace

#endif //W2RP_WRITER_H_