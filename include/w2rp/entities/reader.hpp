/*
 *
 */

#ifndef W2RP_READER_H_
#define W2RP_READER_H_

#include <math.h>
#include <chrono>
#include <string>

#include <w2rp/writerProxy.hpp>
#include <w2rp/helper/fragmentation.hpp>
#include <w2rp/helper/safe_queue.hpp>
#include <w2rp/messages/messages.hpp>
#include <w2rp/comm/socketEndpoint.hpp>
#include <w2rp/comm/UDPComm.hpp>
#include <w2rp/config/readerConfig.hpp>
#include <w2rp/guid/guidPrefix.hpp>
#include <w2rp/guid/guidPrefixManager.hpp>
#include <w2rp/guid/guid.hpp>

namespace w2rp {

class Reader
{
  public:
    /**
     * @brief empty default constructor
     */ 
    Reader();

    /**
     * @brief Construct a new Writer object
     * 
     * @param participant_id
     * @param cfg 
     */
    Reader(uint16_t participant_id, config::readerCfg &cfg);

    /**
     * @brief default destructor
     */
    ~Reader();

    /**************/
    /* Public API */
    /**************/

    /**
     * @brief API for app to retrieve new samples once available
     * 
     * @param data reference to object where sample data shall be written to
     */ 
    void retrieveSample(SerializedPayload &data);

  private:

    // init state
    bool initialized = false;

    // reader configuration
    config::readerCfg config;

    // rtps guid
    GUID_t guid;

    /**********************************/
    /* protocol management structures */
    /**********************************/

    /// HistoryCache
    WriterProxy *writerProxy;

    /**************************/
    /* Comm and message stuff */
    /**************************/

    /// (sub)message parser and encoder
    NetMessageParser *netParser;

    /// UDP Comm abstraction
    UDPComm *CommInterface;

    /// socket receive thread
    std::thread recvThread;
    /// receive handler thread
    std::thread handlerThread;

    /// receive queue
    SafeQueue<MessageNet_t> receiveQueue;

    /// sample queue for application interface
    SafeQueue<SerializedPayload> sampleQueue;

    /********/
    /* misc */
    /********/

    uint32_t nackCount;

    uint32_t debugCnt;

    /********************************************/
    /** Callbacks triggered by external events **/ 
    /********************************************/

     /**
     * @brief Blocking reception of incoming message at socket, put into receiveQueue
     */
    void receiveMsg();

    /**
     * @brief Processing of any message in receiveQueue 
     */
    void handleMsg();

    /**
     * @brief Callback 
     *
     * @param net MessageNet_t object containing W2RP submessages
     * @return true: success, false: failure
     */
    bool handleMessages(MessageNet_t *net);

    /**
     * @brief Callback for processing Data Frag
     *
     * @param msg data frag submessage
     * @return true: success, false: failure
     */
    bool handleDataFrag(DataFrag *msg);

    /**
     * @brief Method for reacting to Heartbeatfrags, send Nackfrag in return
     *
     * @param msg heartbeat submessage
     * @return true: success, false: failure
     */
    bool handleHBFrag(HeartbeatFrag *msg);


    /****************************************************/
    /* methods used during hanlding of Data and HBFrags */
    /****************************************************/

    /**
     * @brief create serialized sample data from all fragments of the sample
     * 
     * @param cfw pointer to history cache object (sample)
     * @param sampleData reference to object where sample data shall be written to
     */ 
    void buildSerializedSample(ChangeForWriter *cfw, SerializedPayload &sampleData);


    /*************************************************/
    /* methods for checking validity of cacheChanges */ 
    /*************************************************/

    /**
     * Method for evaluating whether a sample is still valid or whether its deadline elapsed.
     * Handles removing of sample in case of elapsed deadline
     */
    void checkSampleLiveliness();

  
    /***************************/
    /* miscellaneous functions */ 
    /***************************/
    
    /**
     * @brief Set the Config object
     * 
     * @param cfg writer config object
     */
    void setConfig(config::readerCfg &cfg);

    /**
     * @brief 
     * 
     * @param participant_id 
     */
    void init(uint16_t participant_id);

    /**
     * @brief 
     * 
     * @param participant_id 
     * @param cfg 
     */
    void init(uint16_t participant_id, config::readerCfg &cfg);


};



} //end namespace

#endif //W2RP_READER_H_