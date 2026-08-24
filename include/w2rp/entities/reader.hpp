#ifndef W2RP_READER_H_
#define W2RP_READER_H_

#include <math.h>
#include <chrono>
#include <string>
#include <stdexcept>

#include <boost/asio.hpp>

#include <w2rp/writerProxy.hpp>
#include <w2rp/helper/safe_queue.hpp>
#include <w2rp/helper/circularBuffer.hpp>
#include <w2rp/messages/messages.hpp>
#include <w2rp/messages/messageUtils.hpp>
#include <w2rp/comm/socketEndpoint.hpp>
#include <w2rp/comm/UDPComm.hpp>
#include <w2rp/config/readerConfig.hpp>
#include <w2rp/guid/guidPrefix.hpp>
#include <w2rp/guid/guidPrefixManager.hpp>
#include <w2rp/guid/guid.hpp>
#include <w2rp/config/config.hpp>

namespace w2rp {

typedef std::vector<unsigned char> SerializedPayload;

class Reader
{
public:
    typedef std::pair<uint64_t, SerializedPayload> sample;
    typedef std::pair<MessageNet_t, boost::asio::ip::udp::endpoint> message_pair;
    
    /**
     * @brief Construct a new Writer object
     * 
     * @param participant_id
     * @param cfg 
     */
    Reader(std::uint16_t participant_id, config::readerCfg cfg);

    /**
     * @brief default destructor
     */
    ~Reader();

    /* ------------------------------- Public API ------------------------------- */

    /**
     * @brief API for app to retrieve new samples once available
     * 
     * @param data reference to object where sample data shall be written to
     */ 
    void retrieveSample(sample &data );

private:

    // reader configuration
    config::readerCfg config;

    // rtps guid
    GUID_t guid;

    /* --------------------- protocol management structures --------------------- */

    // storing the "actual" data that is transmitted via RTPS
    stableCircularBuffer<CacheChange> historyCache;

    // the writer proxy keeps track of sending/acknowledgment states for each writer
    WriterProxy writerProxy;


    /* --------------------- Communication and Message Types -------------------- */

    // UDP Comm abstraction
    UDPComm CommInterface;

    // socket receive thread
    std::thread recvThread;
    // receive handler thread
    std::thread handlerThread;

    // receive queue
    SafeQueue<message_pair> receiveQueue;

    // sample queue for application interface
    SafeQueue<std::pair<uint64_t ,SerializedPayload>> sampleQueue;

    /* ------------------------------- statistics ------------------------------- */

    uint32_t nackCount;

    uint32_t debugCnt;


    /* --------------------------- Message Processing --------------------------- */

	struct MessageProcessor
	{
		Reader* readerInstance;
		MessageProcessor(Reader* reader): readerInstance(reader) {}

		void operator()(DataFrag& frag)
		{
			readerInstance->handleDataFrag(&frag);
		}

		void operator()(HeartbeatFrag& frag)
		{
			readerInstance->handleHBFrag(&frag);
		}

        void operator()(NackFrag& frag)
        {
            // NackFrags are not expected at the Reader
        }
	};

     /**
     * @brief Blocking reception of incoming message at socket, put into receiveQueue
     */
    void receiveMessage();
	
    /**
     * @brief Handle incoming messages from the receiveQueue
     * 
     */
    void handleMessage();

    /**
     * @brief Callback for processing Data Frag
     *
     * @param msg data frag submessage
     */
    void handleDataFrag(DataFrag *msg);

    /**
     * @brief Method for reacting to Heartbeatfrags, send Nackfrag in return
     *
     * @param msg heartbeat submessage
     * @return true: success, false: failure
     */
    void handleHBFrag(HeartbeatFrag *msg);

};



} //end namespace

#endif //W2RP_READER_H_