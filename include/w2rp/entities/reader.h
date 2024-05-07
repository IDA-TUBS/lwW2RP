/*
 *
 */

#ifndef W2RP_READER_H_
#define W2RP_READER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <w2rp/writerProxy.h>
#include <w2rp/messages/messages.h>

namespace w2rp {

struct readerCfg
{
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration responseDelay;
    std::string writerAddresses;
    unsigned int sizeCache;
    uint8_t readerUuid;
    uint32_t priority;
};


class Reader
{
  public:
    /*
     * default constructor
     */
    Reader(/* args */);

    /*
     * default destructor
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

    /********/
    /* misc */
    /********/

    uint32_t nackCount;

    /********************************************/
    /** Callbacks triggered by external events **/ 
    /********************************************/

    /*
     * @brief Callback 
     *
     * @param data frag
     * @return success
     */
    bool handleMessages(/* TODO msg*/);

    /*
     * @brief Callback for processing Data Frag
     *
     * @param data frag submessage
     * @return success
     */
    bool handleDataFrag(DataFrag *msg);

    /*
     * @brief Method for reacting to Heartbeatfrags, send Nackfrag in return
     *
     * @param heartbeat message
     * @return success
     */
    bool handleHBFrag(HeartbeatFrag *msg);


    /****************************************************/
    /* methods used during hanlding of Data and HBFrags */
    /****************************************************/


    /*************************************************/
    /* methods for checking validity of cacheChanges */ 
    /*************************************************/

    /*
     * Method for evaluating whether a sample is still valid or whether its deadline elapsed.
     * Handles removing of sample in case of elapsed deadline
     */
    void checkSampleLiveliness();
};



} //end namespace

#endif //W2RP_READER_H_