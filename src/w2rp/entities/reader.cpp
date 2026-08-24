#include <w2rp/entities/reader.hpp>

#include <sstream>
#include <bitset>

namespace w2rp {

Reader::Reader(std::uint16_t participant_id, config::readerCfg cfg)
:
    config(cfg),
    CommInterface(
        socket_endpoint{"0.0.0.0", cfg.endpoint().port},
        config.writer()
    ),
    writerProxy(config.writer_id(), cfg.sizeCache()),
    historyCache(cfg.sizeCache())
{
    //generate guid prefix
    GuidPrefix_t guidPrefix;
    guidPrefixManager::instance().create(
        config.host_id(), 
        participant_id, 
        guidPrefix
    );

    // set guid
    guid = GUID_t(guidPrefix, c_entityID_reader);
    logInfo("[READER] GUID: " << guid);

    // create receive and receive handler threads
    recvThread = std::thread{&Reader::receiveMessage, this};
    handlerThread = std::thread{&Reader::handleMessage, this};

    this->nackCount = 0;
    this->debugCnt = 0;

    logDebug(
        "Reader: " << CommInterface.getRxEndpoint().ip_addr 
        << ":" << CommInterface.getRxEndpoint().port
    )
    
    logDebug(
        "Writer: " << CommInterface.getTxEndpoint().ip_addr 
        << ":" << CommInterface.getTxEndpoint().port
    )

    logInfo("[Reader] finished constructor call")
}

Reader::~Reader()
{}

/********************************************/
/** Callbacks triggered by external events **/ 
/********************************************/

void Reader::receiveMessage()
{      
    // msg to store received data
    MessageNet_t msg;

    udp::endpoint writer;
    while(true)
    {      
        writer = CommInterface.receiveMsg(msg);
        CommInterface.setTxEndpoint(writer);
        receiveQueue.enqueue(std::make_pair(msg, writer));
        // logDebug("[Reader] received and enqueued message")
    }
}

void Reader::handleMessage()
{
    // msg to store received data
    message_pair pair;
    W2RPHeader header;
    std::vector<SubmessageType> submessages;

    while(true)
    {
        pair = receiveQueue.dequeue();

        header.netToHeader(&(pair.first));
        submessages = messageUtils::getSubmessages(&(pair.first));

        MessageProcessor processor(this);

        for(auto& subMsg : submessages)
        {
            std::visit(processor, subMsg);
        }
    }
}

void Reader::handleDataFrag(DataFrag *msg)
{
    if(!historyCache.contains(msg->writerSequenceNumber))
    {
        logDebug("[Reader] handleDataFrag() - creating new CacheChange for SN: " << msg->writerSequenceNumber)
        logTrace("SN New," << msg->writerSequenceNumber)
        // create new change
        CacheChange newChange(
            msg->writerSequenceNumber, 
            msg->sampleSize, 
            msg->fragmentSize, 
            msg->timestamp
        );

        // Simplification for PoC: remove oldest change only if cache full.
        // In a complete implementation, we would check for expired samples as well.
        if(historyCache.size() == historyCache.capacity())
        {
            logDebug("[Reader] handleDataFrag() - removing oldest CacheChange for SN: " << historyCache.front().sequenceNumber)
            writerProxy.removeChange(historyCache.front().sequenceNumber);
        }

        historyCache.push_back(newChange.sequenceNumber, std::move(newChange));
        if(!writerProxy.addChange(historyCache.back()))
        {
            logError("[Reader] handleDataFrag() - failed to add new change to writerProxy for SN: " << msg->writerSequenceNumber)
        }
    }
    else if(writerProxy.isComplete(msg->writerSequenceNumber))
    {
        logDebug("[Reader] handleDataFrag() - received fragment for already complete change SN: " << msg->writerSequenceNumber << ", Fragment: " << msg->fragmentNumber)
        return;
    }
    
    // get existing change
    CacheChange& change = historyCache.lookup(msg->writerSequenceNumber);

    change.updateFragment(
        msg->fragmentNumber, 
        msg->serializedPayload.data(), 
        msg->fragmentSize
    );

    WriterProxy::ChangeStatus status = writerProxy.updateFragmentStatus(
        msg->writerSequenceNumber, 
        msg->fragmentNumber
    );
    
    logDebug("[Reader] handleDataFrag() - updated fragment status for SN: " << msg->writerSequenceNumber << ", Fragment: " << msg->fragmentNumber)
    logTrace("FN Recv," << msg->writerSequenceNumber << "," << msg->fragmentNumber)
    
    if(status == WriterProxy::ChangeStatus::COMPLETE && !writerProxy.checkPublished(msg->writerSequenceNumber))
    {
        SerializedPayload sampleData;
        change.serializeSample(sampleData);
        sampleQueue.enqueue(std::make_pair(change.sequenceNumber, std::move(sampleData)));
        writerProxy.setPublished(change.sequenceNumber);
        logInfo("[Reader] handleDataFrag() - sample complete SN: " << msg->writerSequenceNumber)
        logTrace("SN Complete," << msg->writerSequenceNumber)
    }
}

void Reader::handleHBFrag(HeartbeatFrag *msg)
{
    ChangeFromWriter change = writerProxy.getChange(msg->writerSequenceNumber);

    NackFrag::fragmentNumberSet status;
    status.base = change.getMissingFragmentMapBase();

    // Determine number of bytes to copy
    // -(firstMissingFragment - 1) = Count of continous acked fragments (bits), starting from the first fragment
    // +7 to achieving ceiling
    size_t byteCount = (change.getFragmentCount() - (change.getMissingFragmentMapBase() - 1) + 7) / 8;
    if(byteCount > NACK_BITMAP_SIZE)
    {
        byteCount = NACK_BITMAP_SIZE;
    }

    if(byteCount > 0)
    {
        memcpy(
            status.bitmap, 
            change.getMissingFragmentMap(),
            byteCount
        );
    }

    W2RPHeader header(guid.prefix);
    NackFrag response(
        config.host_id(), 
        writerProxy.writerID,
        msg->writerSequenceNumber,
        status,
        this->nackCount
    );
    this->nackCount++;

    MessageNet_t txMsg;
    header.headerToNet(&txMsg);
    response.nackToNet(&txMsg);

    // send message via UDP
    CommInterface.sendMsg(txMsg);
    logDebug(
        "[Reader] handleHBFrag() - sent NackFrag: SN " << msg->writerSequenceNumber 
        << ", lowestFN " << status.base 
        << ", highestFN " << (status.base + NACK_BITMAP_SIZE*8 - 1)
        << " to Writer " << header.guidPrefix.get_host()
    )
    logTrace("FN NACK," << msg->writerSequenceNumber << "," << status.base)
}

/**************/
/* public API */
/**************/

void Reader::retrieveSample(sample &data)
{    
    data = sampleQueue.dequeue();
}

} //end namespace
