
#ifndef W2RP_WRITER_H_
#define W2RP_WRITER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <w2rp/readerProxy.h>
#include <w2rp/helper/fragmentation.h>

namespace w2rp {

struct writerCfg
{
    uint32_t fragmentSize;
    std::chrono::system_clock::duration deadline;
    std::chrono::system_clock::duration shapingTime;
    std::chrono::system_clock::duration nackSuppressionDuration;
    uint8_t numberReaders;
    std::vector<std::string> readerAddresses;
    unsigned int sizeCache;
};


class Writer
{
private:
    /// writer configuration
    writerCfg config;

    /// sample fragmenter
    Fragmentation *sampleFragmenter;

    /// storing the "actual" data that is transmitted via RTPS
    std::list<CacheChange*> historyCache;
    /// the reader proxies keep track of sending/acknowledgment states for each reader
    std::vector<ReaderProxy*> matchedReaders;
    /// list of fragments to send next
    std::list<SampleFragment*> sendQueue;

    /// Timed Event for resetting fragments
    // TimedEvent* timeoutTimer;
    /// Timed Event for periodic shaping and sample transmissions
    // TimedEvent* shapingTimer;

    /// counter for next sample's sequence number
    uint32_t sequenceNumberCnt;

public:
    /*
     * @brief empty default constructor
     */ 
    Writer(/* args */); // TODO

    /*
     * @brief empty default destructor
     */
    ~Writer(); // TODO

protected:
    /********************************************
     ** Callbacks triggered by external events ** 
     ********************************************/

    /*
     * @brief Callback for creating new cache change on arrival of new sample from application
     *
     * @param data (in serialized form) of the sample received from the application
     */
    bool handleNewSample(SerializedPayload* data);

    /*
     * @brief Method for reacting to NackFrags received from readers
     *
     * @param nackFrag message containing the ack/nack bitmap
     */
    void handleNackFrag(/* TODO nackfrag data*/);


    /*********************************************
     * methods used during fragment transmission * 
     *********************************************/

    /*
     * @brief callback that is triggered according to some schedule. At the end,
     *   sends a packaged sample fragment down towards the UDP/IP stack
     *
     * @return true on success, else false
     */
    bool sendMessage();

    /*
     * @brief Method for selecting a reader for the next transmission
     *
     * @return reader proxy
     */
    ReaderProxy selectReader();

    /*
     * @brief Method for selecting which fragment (missing at a specific reader) to transmit next
     *
     * @param rp pointer to reader proxy used for fragment selection
     * @return the sample fragment to be transmitted next
     */
    SampleFragment* selectNextFragment(ReaderProxy *rp);


    /*************************************************
     * methods for checking validity of cacheChanges * 
     ************************************************/
 
    /*
     * @brief Method for evaluating whether a sample is still valid or whether its deadline elapsed.
     * Handles removing of sample in case of elapsed deadline
     */
    void checkSampleLiveliness();
    

    /*
     * @brief Method for removing samples from the history cache in case the sample has been successfully
     * transmitted to ALL matched readers
     */
    void removeCompleteSamples();

    /*****************************
     * timeout related functions * 
     ****************************/

    /*
     * @brief callback for handling of timeouts
     */
    void handleTimeout();


    /***************************
     * miscellaneous functions * 
     **************************/

}   


} //end namespace

#endif //W2RP_WRITER_H_