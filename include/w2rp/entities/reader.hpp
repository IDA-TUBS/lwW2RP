/*
 *
 */

#ifndef W2RP_READER_H_
#define W2RP_READER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <w2rp/writerProxy.hpp>
#include <w2rp/messages/messages.hpp>

namespace w2rp {

struct readerCfg
{
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration responseDelay;
    std::string writerAddress;
    uint16_t port;
    unsigned int sizeCache;
    uint8_t readerUuid;
    unsigned char guidPrefix[12];
    uint32_t priority;
};

class Reader
{
  public:
    /**
     * @brief default constructor
     */
    Reader(/* args */);

    /**
     * @brief default destructor
     */
    ~Reader();

  private:
    /// reader configuration
    readerCfg config;

    /**********************************/
    /* protocol management structures */
    /**********************************/

    /// HistoryCache
    WriterProxy *writerProxy;

    /******************/
    /* Message parser */
    /******************/

    NetMessageParser *netParser;

    /********/
    /* misc */
    /********/

    uint32_t nackCount;

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


    /*************************************************/
    /* methods for checking validity of cacheChanges */ 
    /*************************************************/

    /**
     * Method for evaluating whether a sample is still valid or whether its deadline elapsed.
     * Handles removing of sample in case of elapsed deadline
     */
    void checkSampleLiveliness();
};



} //end namespace

#endif //W2RP_READER_H_