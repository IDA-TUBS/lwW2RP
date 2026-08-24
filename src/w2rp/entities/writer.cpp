#include <w2rp/entities/writer.hpp>
#include <stdexcept>

#include <sstream>
#include <bitset>

namespace w2rp {

Writer::Writer(std::uint16_t participant_id, config::writerCfg cfg)
:
    config(cfg),
    matchedReader(
        config.reader_id().at(0), 
        cfg.sizeCache(), 
        cfg.nackSuppressionDuration(), 
        cfg.deadline(),
        cfg.shapingTime(),
        cfg.aggregation_size()
    ),
    historyCache(cfg.sizeCache()),
    CommInterface(config.endpoint(), config.readerEndpoint(0)),
    shapingTimer(
        timer_manager,
        cfg.shapingTime(),
        std::bind(&Writer::sendMessage, this),
        NANOSLEEP_TIMER,
        false,
        false
    ),
    timeoutTimer(
        timer_manager,
        std::bind(&Writer::handleTimeout, this)
    )
{
    // generate guid prefix
    GuidPrefix_t guidPrefix;
    guidPrefixManager::instance().create(
        config.host_id(),
        participant_id,
        guidPrefix
    );

    // set guid
    guid = GUID_t(guidPrefix, c_entityID_writer);
    logDebug("[WRITER] GUID: " << guid);

    // create receive and receive handler threads
    writerThread = std::thread{&Writer::sampleProcessing, this};
    recvThread = std::thread{&Writer::receiveMessage, this};
    handlerThread = std::thread{&Writer::handleMessage, this};

    timer_manager.start();

    currentSampleNumber = -1;
    fragmentCounter = 0;
    hbCounter = 0;
    sequenceNumberCnt = 0;

    logInfo("[Writer] finished initialization")
}

Writer::~Writer()
{}

/********************************************/
/** Callbacks triggered by external events **/
/********************************************/

void Writer::receiveMessage()
{
    pthread_setname_np(pthread_self(), "recvThread");
    // msg to store received data
    MessageNet_t msg;
    udp::endpoint reader;

    while(true)
    {
        reader = CommInterface.receiveMsg(msg);
        receiveQueue.enqueue(msg);
    }
}

void Writer::handleMessage()
{
    pthread_setname_np(pthread_self(), "handlerThread");
    MessageNet_t msg;
    W2RPHeader header;
    std::vector<SubmessageType> submessages;

    while(true)
    {
        msg = receiveQueue.dequeue();        
        header.netToHeader(&msg);
        submessages = messageUtils::getSubmessages(&msg);

        NackProcessor processor(this, &header);

        for(auto& subMsg : submessages)
        {
            std::visit(processor, subMsg);
        }
    }
}

void Writer::write(SerializedPayload& data)
{
    sampleQueue.enqueue(data);
}

void Writer::sampleProcessing()
{
    pthread_setname_np(pthread_self(), "writerThread");

    // Pin this thread to Core 4 (index 4)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    SerializedPayload data;
    while(true)
    {
        data = sampleQueue.dequeue();
        handleNewSample(data);
    }
}

bool Writer::handleNewSample(SerializedPayload& data)
{
    auto timestamp_now = std::chrono::system_clock::now();
    logTrace("SN Arrival," << (sequenceNumberCnt+1) << "," << data.size())
    
    if(!addSampleToCache(data, timestamp_now))
    {
        return false;
    }

    // check liveliness of sample in history cache, removes outdated samples
    checkSampleLiveliness();

    // check whether a sample has been successfully transmitted to all readers
    removeCompleteSamples();

    return true;
}

bool Writer::addSampleToCache(SerializedPayload& data, std::chrono::system_clock::time_point timestamp)
{
    std::unique_lock<std::mutex> lock(history_mutex);
    // Assumption for PoC: samples in cache will be outdated before cache is full.
    // Outdated samples are removed in checkSampleLiveliness() and removeCompleteSamples() as part of handleNewSample()
    if(historyCache.size() == historyCache.capacity())
    {
        logDebug("[Writer] history full")
        return false;
    }
    
    // create CacheChange object
    CacheChange newChange(data, ++sequenceNumberCnt, data.size(), config.fragmentSize(), timestamp, config.deadline());
    logDebug(
        "[Writer] created CacheChange " << sequenceNumberCnt 
        << ":" << newChange.numberFragments 
        << " fragments; data length: " << data.size()
        << " lifetime: " << std::chrono::duration_cast<std::chrono::milliseconds>(newChange.lifeTime).count() << " ms"
    )
    
    // add to history cache
    historyCache.push_back(newChange.sequenceNumber, std::move(newChange));

    // generate ChangeForReaders based on CacheChange and add to reader proxy (done by ReaderProxy itself)
    matchedReader.addChange(historyCache.back());

    lock.unlock();

    // start shaping timer for sample transmission to begin
    bool immediateExecution = true;
    shapingTimer.restart(immediateExecution);

    return true;
}

void Writer::handleNackFrag(W2RPHeader *header, NackFrag *msg)
{
    logDebug(
        "[Writer] handleNackFrag() - received NackFrag: SN " << msg->writerSequenceNumber 
        << ", lowestFN " << msg->fragmentStatus.base 
        << ", highestFN " << (msg->fragmentStatus.base + NACK_BITMAP_SIZE*8 - 1)
        << " from Reader " << header->guidPrefix.get_host()
    )
    logTrace("FN NACK," << msg->writerSequenceNumber << "," << msg->fragmentStatus.base)

    // Simplification: One reader per host -> Identify matched readers based on host ID
    std::uint32_t readerID = header->guidPrefix.get_host();

    // tracing info
    uint64_t frag_num = 0;

    if(matchedReader.readerID == readerID)
    {
        switch(matchedReader.processNack(msg))
        {
            case ReaderProxy::CHANGE_NOT_FOUND:
                logDebug("[Writer] handleNackFrag() -  CHANGE_NOT_FOUND")
                logTrace("SN NotFound," << msg->writerSequenceNumber)
                break;
            case ReaderProxy::CHANGE_INCOMPLETE:
                /** 
                 * ! NOTE: The Deadline check is disabled here for the PoC, as the NackFrag reception is to slow 
                 * ! to react to missing fragments during the retransmission pahse. 
                 * ! Hence, pro active retransmission scheduling based on a fragment transmission budget is implemented.
                 */
                // if(isDeadlineAchievable(frag_num))
                // {
                //     if(!shapingTimer.isActive())
                //     {
                //         shapingTimer.restart();
                //     }
                // }
                // else{
                //     logInfo("[Writer] handleNackFrag() -  Sample " << msg->writerSequenceNumber << " cannot meet deadline, removing");
                //     logTrace("SN DeadlineMissed," << msg->writerSequenceNumber << "," << frag_num)
                //     removeCurrentSample();
                // }
                logDebug("[Writer] handleNackFrag() -  CHANGE_INCOMPLETE")
                break;
            case ReaderProxy::CHANGE_EXHAUSTED_RETRY_LIMIT:
                logDebug("[Writer] handleNackFrag() -  CHANGE_EXHAUSTED_RETRY_LIMIT")
                break;
            case ReaderProxy::CHANGE_COMPLETE:
                logDebug("[Writer] handleNackFrag() -  CHANGE_COMPLETE")
                logTrace("SN Complete," << msg->writerSequenceNumber)
                removeCurrentSample();
                break;
            default:
                logDebug("[Writer] handleNackFrag() -  UNKNOWN STATUS")
                break;
        }
    }
}

/*********************************************/
/* methods used during fragment transmission */
/*********************************************/
bool Writer::sendMessage()
{
    // take timestamp for adjusting shaping cycle to properly match configured shaping time
    auto ts_start = std::chrono::steady_clock::now();

    /***********************************************************************************************************************************************/
    /*************************************************** BEGIN CRITICAL SECTION *******************************************************************/
    std::unique_lock<std::mutex> lock(history_mutex);
    // if no sample left to transmit: no need to transmit anything or schedule a new transmission
    if(historyCache.size() == 0)
    {
        logDebug("[Writer] sendMessage() -  history empty, halt shaping")
        logTrace("SN NoSample, 0")
        return false;
    }

    // Check current sample number to update out-dated pointer values if history.front() has changed
    if(currentSampleNumber != historyCache.front().sequenceNumber)
    {
        logInfo("[Writer] sendMessage() - transmitting sample: " << historyCache.front().sequenceNumber)
        currentSampleNumber = historyCache.front().sequenceNumber;
        logTrace("SN START," << currentSampleNumber << "," << historyCache.front().numberFragments)
        currentChange = &historyCache.front();
        currentChangeForReader = matchedReader.getChange(currentChange->sequenceNumber);
    }

    // create W2RP header
    W2RPHeader header(guid.prefix);
    HeartbeatFrag* hbFrag = nullptr;
    MessageNet_t txMsg;

    logDebug("[Writer] sendMessage() - executing send routine")
    
    for(std::uint32_t i = 0; i < config.aggregation_size(); i++)
    {
        std::uint64_t fragmentNumber = currentChangeForReader->getFragmentForTx();
        SampleFragment* sf = nullptr;
        if (fragmentNumber != 0)
        {
            sf = currentChange->getFragment(fragmentNumber);
        }
        else
        {
            logDebug("[Writer] sendMessage() -  No fragment available for transmission, end aggregation")
            logTrace("SN NoFragment," << currentChange->sequenceNumber)
            break;
        }

        // Simplification: One reader per host -> Identify matched readers based on host ID
        DataFrag data(config.host_id(), matchedReader.readerID, *currentChange, *sf);

        // Check if heartbeat needs to be sent
        if((i == (config.aggregation_size() - 1)) || (currentChangeForReader->unsentCount() == 0))
        {
            HeartbeatFrag hb(
                config.host_id(), 
                matchedReader.readerID, 
                currentChange->sequenceNumber, 
                currentChangeForReader->highestSent, 
                hbCounter++
            );
            hbFrag = &hb;
        }
        else{
            hbFrag = nullptr;
        }
        
        // serialization (toNet) and concatenation of submessages
        messageUtils::buildDataMessage(&txMsg, &header, &data, hbFrag);
        CommInterface.sendMsg(txMsg);
        logDebug(
            "[Writer] sendMessage() -  sent DataFrag: SN " << currentChange->sequenceNumber 
            << ", Fragment " << sf->number 
            << " to Reader " << matchedReader.readerID
        )
        logTrace("FN Sent," << currentChange->sequenceNumber << "," << sf->number)
    }
    lock.unlock();
    /*************************************************** END CRITICAL SECTION ********************************************************************/
    /***********************************************************************************************************************************************/

    if(currentChangeForReader->unsentCount() < config.aggregation_size())
    {
        logDebug("[Writer] sendMessage() -  less fragments left than aggregation size, schedule timeout.")
        // configure timeout for reset of timed-out fragments and retransmission, if needed
        auto reset_timepoint = std::chrono::system_clock::now() + config.timeout();
        matchedReader.setTimeout(reset_timepoint, currentSampleNumber);
        if(!timeoutTimer.isActive())
        {
            timeoutTimer.restart(reset_timepoint);
        }
        
        if(currentChangeForReader->unsentCount() == 0)
        {
            logDebug("[Writer] sendMessage() -  all fragments sent for current sample")
        }
    }

    if(!currentChange->isValid())
    {
        logInfo("[Writer] sendMessage() -  Sample " << currentSampleNumber << " cannot meet deadline, removing");
        logTrace("SN DeadlineMissed," << currentSampleNumber << "," << currentChangeForReader->unsentCount())
        removeCurrentSample();
    }

    // continue shaping timer for next transmission opportunity
    return true;
}

/*************************************************/
/* methods for checking validity of cacheChanges */
/*************************************************/

void Writer::checkSampleLiveliness()
{
    std::unique_lock<std::mutex> lock(history_mutex); // lock history access for the duration of the modification

    while(!historyCache.empty())
    {
        auto& change = historyCache.front();
        if(!change.isValid())
        {
            logDebug("[Writer] checkSampleLiveliness() - delete change " << change.sequenceNumber << " from history")
            matchedReader.removeChange(change.sequenceNumber);
            historyCache.pop_front();
        }
        else
        {
            logDebug("[Writer] checkSampleLiveliness() - change " << change.sequenceNumber << " is still valid - stop checking further samples")
            break;
        }
    }

    lock.unlock(); // modification complete, unlock history
}

void Writer::removeCompleteSamples()
{
    // iterate over all changes, remove those that are complete at the reader
    std::unique_lock<std::mutex> lock(history_mutex); 

    while(!historyCache.empty())
    {
        auto& change = historyCache.front();

        if(matchedReader.checkSampleCompleteness(change.sequenceNumber))
        {
            // remove if change successfully acknowledged by reader
            logDebug("[Writer] removeCompleteSamples() - delete change " << change.sequenceNumber << " from history")
            matchedReader.removeChange(change.sequenceNumber);
            historyCache.pop_front();
        }
        else
        {
            // assumption here: samples put into history cache in the right order AND
            // a consecutive sample will only be transmitted after a previous sample is
            // either complete or its deadline expired. Hence, there is no need to check
            // further samples for completeness if there is any incomplete sample
            break;
        }
    }
    lock.unlock();
}

void Writer::removeCurrentSample()
{
    std::unique_lock<std::mutex> lock(history_mutex); 
    if(!historyCache.empty())
    {
        auto& change = historyCache.front();
        logDebug("[Writer] removeCompleteSample_impl() - delete change " << change.sequenceNumber << " from history")
        matchedReader.removeChange(change.sequenceNumber);
        historyCache.pop_front();
    }
    lock.unlock();
}

bool Writer::isDeadlineAchievable(uint64_t& frag_num)
{
    auto now = std::chrono::system_clock::now();

    std::unique_lock<std::mutex> lock(history_mutex);
    auto deadline_time = currentChange->arrivalTime + config.deadline();
    uint64_t fragments_left = currentChangeForReader->unsentCount();
    lock.unlock();

    if(fragments_left == 0)
    {
        frag_num = 0;
        return true;
    }
    if (now >= deadline_time) return false;

    auto time_remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline_time - now);
        
    // Shaping time is the delay between transmissions. 
    // Total time needed = ceil(fragments / aggregation_size) * shaping_interval
    auto num_transmissions = (fragments_left + config.aggregation_size() - 1) / config.aggregation_size();
    auto time_needed = num_transmissions * config.shapingTime();
    
    // tracing info
    frag_num = fragments_left;
    return time_needed <= time_remaining;
}


/*****************************/
/* timeout related functions */
/*****************************/

void Writer::handleTimeout()
{
    // tracing info
    uint64_t frag_num = 0;
    switch(matchedReader.checkTimeout())
    {
        case ReaderProxy::NO_TIMEOUT:
            logDebug("[Writer] handleTimeout() - NO_TIMEOUT")
            break;
        case ReaderProxy::TIMEOUT_PENDING:
            logDebug("[Writer] handleTimeout() - TIMEOUT_PENDING")
            timeoutTimer.restart(matchedReader.getTimeout());
            break;
        case ReaderProxy::TIMEOUT_EXPIRED:
            logDebug("[Writer] handleTimeout() - TIMEOUT_EXPIRED")
            if(matchedReader.resetTimedOutFragments())
            {
                /**
                 * ! NOTE: The Deadline check is disabled here for the PoC, as the NackFrag reception is to slow 
                 * ! to react to missing fragments during the retransmission pahse. 
                 * ! Hence, pro active retransmission scheduling based on a fragment transmission budget is implemented.
                 */
                // {
                //     if(!shapingTimer.isActive())
                //     {
                //         shapingTimer.restart();
                //     }
                // }
                // else
                // {
                //     logInfo("[Writer] handleTimeout() -  Sample " << currentSampleNumber << " cannot meet deadline, removing");
                //     removeCurrentSample();
                //     logTrace("SN DeadlineMissed," << currentSampleNumber << "," << frag_num)
                //     if(historyCache.size() > 0 && !shapingTimer.isActive())
                //     {
                //         shapingTimer.restart();
                //     }
                // }
            }
            else{
                logInfo("[Writer] handleTimeout() -  Sample " << currentSampleNumber << " complete, removing");
                removeCurrentSample();
                logTrace("SN Complete," << currentSampleNumber << "," << frag_num)
                if(historyCache.size() > 0 && !shapingTimer.isActive())
                {
                    shapingTimer.restart();
                }
            }
            break;
        default:
            logError("[Writer] handleTimeout() - unknown timeout status")
            break;
    }
}

/************************/
/* RM related functions */ 
/************************/

void Writer::updateTxEndpoint(std::string endpoint_tx, int port)
{
    logInfo("[Writer] updateTxEndpoint() - Updating Tx Endpoint: " << endpoint_tx << ":" << port)
    CommInterface.setTxEndpoint(socket_endpoint(endpoint_tx, port));
}


} //end namespace