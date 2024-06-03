/*
 *
 */

#include <w2rp/entities/writer.hpp>
#include <stdexcept>


namespace w2rp {

Writer::Writer()
{
    logInfo("[Writer] empty constructor call")
}

Writer::Writer(uint16_t participant_id, config::writerCfg &cfg)
:
    config(cfg)
{
    init(participant_id);
    logInfo("[Writer] finished constructor call")
}

Writer::~Writer()
{
    logInfo("[Writer] delete")
    matchedReaders.clear();
    historyCache.clear();
    sendQueue.clear();

    delete netParser;
}

void Writer::init(uint16_t participant_id)
{
    //generate guid prefix
    GuidPrefix_t guidPrefix;
    guidPrefixManager::instance().create(
        config.host_id(), 
        participant_id, 
        guidPrefix
    );


    // set guid
    guid = GUID_t(guidPrefix, c_entityID_writer);

    sequenceNumberCnt = 0;

    // initialize sample fragmenter
    sampleFragmenter = new Fragmentation(config.fragmentSize());

    // reader proxy initialization
    for(u_int32_t i = 0; i < config.numberReaders(); i++) {
        // the app's reader IDs are in the range of [appID * maxNumberReader + 1, (appID + 1) * maxNumberReader - 1]

        ReaderProxy* rp = nullptr;
        if(config.nackSuppressionDuration() != std::chrono::system_clock::duration::zero())
        {
            rp = new ReaderProxy(i, this->config.sizeCache(), config.nackSuppressionDuration());
        }
        else
        {
            rp = new ReaderProxy(i, this->config.sizeCache());
        }
        matchedReaders.push_back(rp);
    }

    // init net message parser
    netParser = new NetMessageParser();

    // init UDPComm object
    socket_endpoint rx_socketEndpoint = config.endpoint();
    socket_endpoint tx_socketEndpoint = config.readerEndpoint(0);
    CommInterface = new UDPComm(rx_socketEndpoint, tx_socketEndpoint);

    // create receive and receive handler threads
    recvThread = std::thread{&Writer::receiveMsg, this};
    handlerThread = std::thread{&Writer::handleMsg, this};


    // init timers
    std::chrono::microseconds cycle = config.shapingTime(); // TODO take cycle time from writer config

    shapingTimer = new PeriodicEvent(
        this->timer_manager, 
        cycle,
        std::bind(&Writer::sendMessage, this)
    );
    // only start timer once data available, hence stop immediately
    shapingTimer->cancel();  

    timeoutTimer = new TimedEvent(
        this->timer_manager,
        std::bind(&Writer::handleTimeout, this)
    );

    timer_manager.start();

    currentSampleNumber = -1;
    fragmentCounter = 0;
    hbCounter = 0;

    initialized = true;

    logInfo("[Writer] finished initialization")
}

void Writer::init(uint16_t participant_id, config::writerCfg &cfg)
{
    setConfig(cfg);
    init(participant_id);    
}

/********************************************/
/** Callbacks triggered by external events **/ 
/********************************************/

void Writer::receiveMsg()
{      
    // msg to store received data
    MessageNet_t msg;
   
    while(true)
    {      
        CommInterface->receiveMsg(msg);

        receiveQueue.enqueue(msg);
    }
}


void Writer::handleMsg()
{    
    // msg to store received data
    MessageNet_t msg;

    while(true)
    {              
        msg = receiveQueue.dequeue();
        
        handleMessages(&msg);
    }
}



bool Writer::handleMessages(MessageNet_t *net)
{
    // first extract submessages from msg
    std::vector<SubmessageBase*> res;

    netParser->getSubmessages(net, &res);
    
    // call corresponding submsg handler functions
    NackFrag* nackFrag;
    for(auto subMsg : res)
    {
        switch (subMsg->subMsgHeader->submessageId)
        {
        case NACK_FRAG:
            logDebug("[Writer] received NackFrag")
            nackFrag = (NackFrag*)(subMsg);
            handleNackFrag(nackFrag);
            break;
        default:
            break;
        }
    }

    return true;
}


bool Writer::write(SerializedPayload *data)
{
    logInfo("[Writer] new sample received")
    if(initialized)
    {
        return handleNewSample(data);
    }
    else
    {
        logError("Writer is uninitialized, no config available")
        throw std::invalid_argument("initialization missing, config not found");
        return false;
    }
}


bool Writer::handleNewSample(SerializedPayload *data)
{
    auto timestamp_now = std::chrono::system_clock::now();

    // first check existing samples for deadline expiry
    checkSampleLiveliness();
    addSampleToCache(data, timestamp_now);

    return true;
}

bool Writer::addSampleToCache(SerializedPayload *data, std::chrono::system_clock::time_point timestamp)
{
    logInfo("[Writer] addSampleToCache")
    // create CacheChange object
    CacheChange *newChange = new CacheChange(sequenceNumberCnt++, data->length, config.fragmentSize(), timestamp);
    // logDebug("[Writer] created CacheChange")

    // fragment sample
    std::vector<SampleFragment*> fragments;
    bool compare = false;
    sampleFragmenter->fragmentPayload(data, newChange, &fragments, timestamp, compare);
    // logDebug("[Writer] fragmented data")
    newChange->setFragmentArray(&fragments);
    // logDebug("[Writer] added fragmented data to change")
    

    // add CacheChange to history
    if(historyCache.size() == this->config.sizeCache())
    {
        logDebug("[Writer] history full")
        return false;
    }
    historyCache.push_back(newChange);
    // logDebug("[Writer] added change to history")
    

    // generate ChangeForReaders based on CacheChange and add to reader proxies (done by ReaderProxy itself)
    for (auto rp: matchedReaders)
    {
        // logDebug("[Writer] adding change to readerProxy: " << unsigned(rp->readerID))
        rp->addChange(*newChange);
    }
    logDebug("[Writer] added change to readerProxies")

    // start shaping timer for sample transmission to begin
    if(!shapingTimer->isActive())
    {
        shapingTimer->restart();
        logDebug("[Writer] started shaping timer")
    }

    return true;
}


void Writer::handleNackFrag(NackFrag *msg)
{
    logDebug("[Writer] handleNackFrag: received NackFrag - readerID: " << unsigned(msg->readerID))
    uint32_t readerID = msg->readerID;

    // assumption and simplifications for PoC: readerID always correspond to index in matchedReaders 
    auto rp = matchedReaders[readerID];

    // only handle NackFrag if sample still in history, if already complete or expired just ignore NackFrag
    if(rp->processNack(msg))
    {
        unsigned int sequenceNumber = msg->writerSN;
        bool complete = rp->checkSampleCompleteness(sequenceNumber);

        // restart shapingTimer if not active any more and sample not yet complete
        if(!shapingTimer->isActive())
        {
            shapingTimer->restart();
        }
    }    
}






/*********************************************/
/* methods used during fragment transmission */
/*********************************************/


bool Writer::sendMessage(){
    // logDebug("[Writer] sendMessage()")
    // check liveliness of sample in history cache, removes outdated samples
    checkSampleLiveliness();

    // check whether a sample has been successfully transmitted to all readers
    removeCompleteSamples();

    // if no sample left to transmit: no need to transmit anything or schedule a new transmission
    if(historyCache.size() == 0)
    {
        // logDebug("[Writer] sendMessage: history empty, halt shaping")
        return false;
    }
    
    if(currentSampleNumber != historyCache.front()->sequenceNumber)
    {
        // logDebug("[Writer] sendMessage: fill send queue with new sample")
        // new sample to transmit
        this->currentSampleNumber = historyCache.front()->sequenceNumber;
        // priming send queue with all fragments of the new sample
        fillSendQueueWithSample(this->currentSampleNumber);
    }

    // differentiate two scenarios:
    // 1. scenario: send queue is empty, select a new fragment for tx or retx
    if(sendQueue.empty())
    {
        // logDebug("[Writer] sendMessage: fill queue for retx")
        // add new fragment to queue
        SampleFragment* sf = nullptr;
        ReaderProxy *rp = nullptr;
        // then select a reader
        if(rp = selectReader())
        {
            // finally select a fragment from the previously chosen reader for transmission

            if(sf = selectNextFragment(rp))
            {
                // add sample fragment to send queue
                // use actual 'data' sample fragment from history cache instead of sf from reader proxy
                auto sfToSend = (sf->baseChange->getFragmentArray())[sf->fragmentStartingNum];
                sendQueue.push_back(sfToSend);
            }
        }
    }
    // 2. scenario: send queue not empty (any more)
    if(!sendQueue.empty())
    {
        // logDebug("[Writer] sendMessage: select fragment for (re)tx")
        auto t_now = std::chrono::system_clock::now();

        SampleFragment* sf = sendQueue.front();
        // update currentSN
        currentChange = sf->baseChange;

        sendQueue.pop_front();
        // update fragment status (at all reader proxies if multicast is used)
        for(auto rp: matchedReaders)
        {
            rp->updateFragmentStatus(SENT, sf->baseChange->sequenceNumber, sf->fragmentStartingNum, t_now);

            // check for timeout situation: reader has no fragments in state 'UNSENT' left
            if(rp->checkForTimeout(sf->baseChange->sequenceNumber))
            {
                // timeout needed

                // determine TO time of fragment
                auto toTS = t_now + config.timeout();
                rp->setTimeoutTimestamp(toTS);

                timeoutQueue.push(rp);

                if(!timeoutTimer->isActive())
                {
                    timeoutTimer->restart(toTS);
                }
            }

        
        
        // create W2RP header
        W2RPHeader *header = new W2RPHeader(guid.prefix);
        // logDebug("[Writer] sendMessage: created W2RP header")
        
        // create submessages: DataFrag and HBFrag
        DataFrag* data;
        HeartbeatFrag* hb;
        
        createDataFrag(sf, data);
        // logDebug("[Writer] sendMessage: created dataFrag")
        // data->print();
        logInfo("[WRITER] tx fragment " << data->fragmentStartingNum << ": " << data->serializedPayload)

        this->createHBFrag(sf, hb);
        // logDebug("[Writer] sendMessage: created HBFrag")

        // serialization (toNet) and concatenation of submessages 
        MessageNet_t *txMsg = new MessageNet_t;
        header->headerToNet(txMsg);
        data->dataToNet(txMsg);
        hb->hbToNet(txMsg);
        // logDebug("[Writer] sendMessage: net message from submessages")

        // send message via UDP
        CommInterface->sendMsg(*txMsg);

        
        delete header;
        delete data;
        delete hb;
        delete txMsg;
        // logDebug("[Writer] sendMessage: delete message")
        }
    }
    // if send queue still empty, no need to schedule new fragment transmission, 
    // wait for next sample top arrive
    else
    {
        logInfo("[Writer] sendMessage: no fragments available, halt shaping")
        // ...
        return false;
    }


    return true;
}

ReaderProxy* Writer::selectReader()
{
    ReaderProxy *nextReader = nullptr;    
    
    // for mode 1:
    uint32_t highestPriority = std::numeric_limits<uint32_t>::max();
    // for mode 2:
    uint32_t leastNacks = std::numeric_limits<uint32_t>::max();
    // for mode 3:
    uint32_t mostNacks = 0;
    // find the highest priority reader that still hasn't received the current sample completely
    for(auto rp: matchedReaders)
    {
        // TODO if needed insert checking for deadline violation condition here:
        // check whether the remaining slot suffice for transmitting sending all
        // remaining fragments: N_f,rem >= N_f,missing

        if(config.prioMode() == FIXED)
        {
            // Prio Mode 1: Using fixed priorities

            // note: if everything is working correctly currentSampleNumber will be also
            // be the sequence number of the first element in the reader proxy's history
            if((rp->priority < highestPriority) && !(rp->checkSampleCompleteness(this->currentSampleNumber)))
            {
                nextReader = rp;
                highestPriority = rp->priority;
            }
        }
        else if(config.prioMode() == ADAPTIVE_LOW_PDR)
        {
            // Prio Mode 2: Use adaptive prioritization based on packet delivery rate (PDR)
            // select the reader with the most negatively acknowledged fragments
            // works best for equal FERs at each reader
            if((rp->getUnsentFragments(this->currentSampleNumber).size() > mostNacks) && !(rp->checkSampleCompleteness(currentSampleNumber)))
            {
                mostNacks = rp->getUnsentFragments(currentSampleNumber).size();
                nextReader = rp;
            }

        }
        else if(config.prioMode() == ADAPTIVE_HIGH_PDR)
        {
            // Prio Mode 3: Use adaptive prioritization based on packet delivery rate (PDR)
            // select the reader with the least amount of negatively acknowledged fragments
            if((rp->getUnsentFragments(this->currentSampleNumber).size() < leastNacks) && !(rp->checkSampleCompleteness(currentSampleNumber)))
            {
                leastNacks = rp->getUnsentFragments(this->currentSampleNumber).size();
                nextReader = rp;
            }
        }
    }
    

    return nextReader;
}

SampleFragment* Writer::selectNextFragment(ReaderProxy *rp)
{
    // used for retransmissions only
    // find the unacknowledged fragment and return that fragment for transmission
    SampleFragment *tmp = nullptr;
    SampleFragment **fragments = rp->getCurrentChange()->getFragmentArray();
    for (int i = 0; i < rp->getCurrentChange()->numberFragments; i++)
    {
        SampleFragment* sf = fragments[i];
        if (sf->sent || sf->acked) {
            continue;
        }

        // take the first unsent and unacknowledged fragment
        tmp = sf;
        break;
    }
    return tmp;
}



void Writer::fillSendQueueWithSample(uint32_t sequenceNumber)
{
    // for multicast all readers need the same data. Just take the first reader here,
    // as it does not make a difference in the transmission phase which on is used
    // for fragment selection - each readers needs each fragment once

    ReaderProxy* rp = matchedReaders[0];

    auto unsentFragments = rp->getUnsentFragments(sequenceNumber);
    // use actual 'data' sample fragment from history cache instead of sf from reader proxy
    for (auto sf: unsentFragments)
    {
        auto sfToSend = (sf->baseChange->getFragmentArray())[sf->fragmentStartingNum];
        sendQueue.push_back(sfToSend);
    }
}


void Writer::createDataFrag(SampleFragment* sf, DataFrag*& ret)
{   
    // ?????????????????????????????????????????
    // TODO borad/multicast readerID = numeric_limits<uint32_t>::max()? message.h #define ID_BROADCAST numeric_limits<uint32_t>::max()
    // uint32_t readerID = 0;
    // ????????????????????????????????????????

    ret = new DataFrag(c_entityID_reader.to_uint32(), c_entityID_writer.to_uint32(),
                        sf->baseChange->sequenceNumber, sf->fragmentStartingNum,
                        sf->baseChange->sampleSize, sf->dataSize, sf->data, sf->baseChange->arrivalTime);
}


void Writer::createHBFrag(SampleFragment* sf, HeartbeatFrag*& ret)
{
    // ????????????????????????????????????????
    // TODO borad/multicast readerID = 0? message.h #define ID_BROADCAST 0
    // uint32_t readerID = 0;
    // ????????????????????????????????????????

    // Hardcoded + static entityIDs, since lightweightW2RP only supports a single type of reader and writer entities. 
    ret = new HeartbeatFrag(c_entityID_reader.to_uint32(), c_entityID_writer.to_uint32(),
                            sf->baseChange->sequenceNumber,
                            matchedReaders[0]->getCurrentChange()->highestFNSend,
                            hbCounter);

    hbCounter++;
}



/*************************************************/
/* methods for checking validity of cacheChanges */ 
/*************************************************/

void Writer::checkSampleLiveliness()
{
    // logDebug("[Writer] checkSampleLiveliness")
    if(historyCache.size() == 0)
    {
        while(sendQueue.size() > 0){
            SampleFragment* to_delete_element = sendQueue.front();
            sendQueue.erase(sendQueue.begin());
            delete to_delete_element;
        }
//        sendQueue.clear();
        return;
    }

    std::vector<uint32_t> deprecatedSNs;
    std::vector<CacheChange*> toDelete;
    // check liveliness of samples in history cache, if deadline expired remove sample from cache and ReaderProxies
    auto* change = historyCache.front();
    while(1)
    {
        if(!change->isValid(this->config.deadline()))
        {
            deprecatedSNs.push_back(change->sequenceNumber);
            historyCache.pop_front();
            toDelete.push_back(change); // delete all expired changes in the end
            if(historyCache.size() == 0)
            {
                // deleted only existing entry in cache
                break;
            }
        }
        // assumption here: samples put into history cache in the right order AND
        // all samples have the same deadline, hence any sample in the history
        // following the first valid sample is also still valid!
        break;
    }

    for (uint32_t sequenceNumber: deprecatedSNs)
    {
        for (auto rp: matchedReaders)
        {
            if(!(rp->changeExists(sequenceNumber)))
            {
                continue;
            }
            rp->removeChange(sequenceNumber);
        }
    }

    if(!sendQueue.empty())
    {
        // also purge fragments of expired samples from sendQueue
        for(auto it = sendQueue.cbegin(); it != sendQueue.end(); it++)
        {
            // remove fragments if their sequence number matches any of those in deprecatedSNs
            uint32_t sequenceNumber = (*it)->baseChange->sequenceNumber;
            for(uint32_t deprecatedSn: deprecatedSNs)
            {
                if(sequenceNumber == deprecatedSn)
                {
                    sendQueue.erase(it--);
                    break;
                }
            }
        }
    }

    // finally delete expired changes
    //toDelete.clear();
    while(toDelete.size() > 0){
        CacheChange* to_delete_element = toDelete.front();
        toDelete.erase(toDelete.begin());
        delete to_delete_element;
    }
}

void Writer::removeCompleteSamples()
{
    if(historyCache.size() == 0)
    {
        return;
    }

    // iterate over all changes, remove those that are complete at ALL (!!) readers
    while(1)
    {
        auto* change = historyCache.front();

        if(!change)
        {
            break;
        }

        bool completed = true;
        for(auto rp: matchedReaders)
        {
            if(!(rp->checkSampleCompleteness(change->sequenceNumber)))
            {
                completed = false;
                break;
            }
        }

        if(completed)
        {
            // remove if change successfully acknowledged by all readers
            for(auto rp: matchedReaders)
            {
                rp->removeChange(change->sequenceNumber);
            }
            historyCache.pop_front();
            // remove all queued fragments of that sample from the send queue
            for(auto it = sendQueue.cbegin(); it != sendQueue.end(); it++)
            {
                // remove fragments if their sequence number matches that of the just removed change
                uint32_t sequenceNumber = (*it)->baseChange->sequenceNumber;

                if(sequenceNumber == change->sequenceNumber)
                {
                    sendQueue.erase(it--);
                    break;
                }

            }
            logDebug("[Writer] delete change " << change->sequenceNumber << " from history")
            delete change;
        }
        else
        {
            // assumption here: samples put into history cache in the right order AND
            // a consecutive sample will only be transmitted after a previous sample is
            // either complete or its deadline expired. Hence, there is no need to check
            // further samples for completeness if there is any incomplete sample
            break;
        }
//        if(historyCache.size() == 0 || change == historyCache.back())
//        {
//            break;
//        }
    }
}


/*****************************/
/* timeout related functions */ 
/*****************************/

void Writer::handleTimeout()
{
    // current timestamp
    auto t_now = std::chrono::system_clock::now();

    ReaderProxy *timeoutedReader = timeoutQueue.front();
    if(timeoutedReader)
    {
        // current (first) change in readerProxy
        timeoutedReader->resetTimeoutedFragments(this->currentChange->sequenceNumber);
        timeoutedReader->timeoutActive = false;
        timeoutQueue.pop();

        if(!shapingTimer->isActive())
        {
            shapingTimer->restart();
        }
    }


    while(!timeoutQueue.empty())
    {
        // reader with the next timeout
        ReaderProxy *nextReader = timeoutQueue.front();

        // first check whether resetting still necessary or whether all fragments are acked by now
        if(nextReader->getAckCount(this->currentChange->sequenceNumber) == this->currentChange->numberFragments) {
            timeoutQueue.pop();
            continue;
        }

        auto t_nextTimeout = nextReader->getTimeoutTimestamp();

        if(t_nextTimeout <= t_now)
        {
            if(nextReader)
            {
                // current (first) change in readerProxy
                nextReader->resetTimeoutedFragments(this->currentChange->sequenceNumber);
                nextReader->timeoutActive = false;
                timeoutQueue.pop();

                if(!shapingTimer->isActive())
                {
                    shapingTimer->restart();
                }
            }
        }
        else
        {
            // timeout may become relevant in the future, restart timedEvent accordingly
            if(!timeoutTimer->isActive())
            {
                timeoutTimer->restart(t_nextTimeout); 
            }
        }
        
        
        break;
    }
}



/***************************/
/* miscellaneous functions */ 
/***************************/

void Writer::setConfig(config::writerCfg &cfg)
{
    config = cfg;
}

} //end namespace