/*
 *
 */

#include <w2rp/entities/writer.hpp>

namespace w2rp {

Writer::Writer()
{
    // TODO fill writer config
    config.fragmentSize = 5;
    config.deadline =  std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5));
    config.shapingTime =  std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::microseconds(10000));
    config.nackSuppressionDuration =  std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::microseconds(20000));
    config.numberReaders = 1;
    config.readerAddress = "127.0.0.1";
    config.readerPort = 1024;
    config.writerAddress = "127.0.0.1";
    config.writerPort = 1025;
    config.sizeCache = 2;
    config.writerUuid = 0;
    memcpy(config.guidPrefix, "_GUIDPREFIX_", 12);
    config.prioMode = ADAPTIVE_HIGH_PDR;

    sequenceNumberCnt = 0;

    // initialize sample fragmenter
    sampleFragmenter = new Fragmentation(config.fragmentSize);

    // reader proxy initialization
    for(u_int32_t i = 0; i < config.numberReaders; i++) {
        // the app's reader IDs are in the range of [appID * maxNumberReader + 1, (appID + 1) * maxNumberReader - 1]

        ReaderProxy* rp = nullptr;
        if(config.nackSuppressionDuration != std::chrono::system_clock::duration::zero())
        {
            rp = new ReaderProxy(i, this->config.sizeCache, config.nackSuppressionDuration);
        }
        else
        {
            rp = new ReaderProxy(i, this->config.sizeCache);
        }
        matchedReaders.push_back(rp);
    }

    // init net message parser
    netParser = new NetMessageParser();

    // init UDPComm object
    socket_endpoint rx_socketEndpoint(config.writerAddress, config.writerPort);
    socket_endpoint tx_socketEndpoint(config.readerAddress, config.readerPort);
    CommInterface = new UDPComm(rx_socketEndpoint, tx_socketEndpoint);


    // init timers
    std::chrono::microseconds cycle(500000); // TODO take cycle time from writer config

    shapingTimer = new PeriodicEvent(
        this->timer_manager, 
        cycle,
        std::bind(&Writer::sendMessage, this)
    );
    // only start timer once data available, hence stop immediately
    shapingTimer->cancel_timer();  

    timer_manager.start();

    currentSampleNumber = -1;
    fragmentCounter = 0;
    hbCounter = 0;

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







/********************************************/
/** Callbacks triggered by external events **/ 
/********************************************/

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
            nackFrag = (NackFrag*)(subMsg);
            handleNackFrag(nackFrag);
            break;
        default:
            break;
        }
    }

    delete nackFrag;
    delete net;
    return true;
}


bool Writer::write(SerializedPayload *data)
{
    logInfo("[Writer] new sample received")
    return handleNewSample(data);
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
    CacheChange *newChange = new CacheChange(sequenceNumberCnt++, data->length, config.fragmentSize, timestamp);
    // logDebug("[Writer] created CacheChange")

    // fragment sample
    std::vector<SampleFragment*> fragments;
    bool compare = false;
    sampleFragmenter->fragmentPayload(data, newChange, &fragments, timestamp, compare);
    // logDebug("[Writer] fragmented data")
    newChange->setFragmentArray(&fragments);
    // logDebug("[Writer] added fragmented data to change")
    


    // add CacheChange to history
    if(historyCache.size() == this->config.sizeCache)
    {
        logDebug("[Writer] history full")
        return false;
    }
    historyCache.push_back(newChange);
    // logDebug("[Writer] added change to history")
    

    // generate ChangeForReaders based on CacheChange and add to reader proxies (done by ReaderProxy itself)
    for (auto rp: matchedReaders)
    {
        rp->addChange(*newChange);
    }
    // logDebug("[Writer] added change to readerProxies")

    // start shaping timer for sample transmission to begin
    if(!shapingTimer->isActive())
    {
        shapingTimer->restart_timer();
        logDebug("[Writer] started shaping timer")
    }

    return true;
}


void Writer::handleNackFrag(NackFrag *msg)
{
    logInfo("[Writer] handleNackFrag: received NackFrag")
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
            shapingTimer->restart_timer();
        }
    }    
}






/*********************************************/
/* methods used during fragment transmission */
/*********************************************/

// bool Writer::timerHandler()
// {
//     logInfo("Test")
//     return true;
// };

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
        sendQueue.pop_front();
        // update fragment status (at all reader proxies if multicast is used)
        for(auto rp: matchedReaders)
        {
            rp->updateFragmentStatus(SENT, sf->baseChange->sequenceNumber, sf->fragmentStartingNum, t_now);

            // TODO timeout stuff
            // // check for timeout situation: reader has no fragments in state 'UNSENT' left
            // if(rp->checkForTimeout(sf->baseChange->sequenceNumber) && !(rp->timeoutActive))
            // {
            //     rp->timeoutActive = true;
            //     // trigger timeout
            //     auto nextTimeout = new Timeout("timeoutEvent");
            //     nextTimeout->setId(rp->getReaderId());
            //     nextTimeout->setSequenceNumber(sf->baseChange->sequenceNumber);

            //     activeTimeouts++;

            //     scheduleAt(simTime() + timeout, nextTimeout);
            // }
        
        // create W2RP header
        W2RPHeader *header = new W2RPHeader(config.guidPrefix);
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

        if(config.prioMode == FIXED)
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
        else if(config.prioMode == ADAPTIVE_LOW_PDR)
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
        else if(config.prioMode == ADAPTIVE_HIGH_PDR)
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
    // TODO borad/multicast readerID = numeric_limits<uint32_t>::max()? message.h #define ID_BROADCAST numeric_limits<uint32_t>::max()
    uint32_t readerID = 0;

    ret = new DataFrag(config.guidPrefix, readerID, config.writerUuid,
                        sf->baseChange->sequenceNumber, sf->fragmentStartingNum,
                        sf->baseChange->sampleSize, sf->dataSize, sf->data, sf->baseChange->arrivalTime);
}


void Writer::createHBFrag(SampleFragment* sf, HeartbeatFrag*& ret)
{
    // TODO borad/multicast readerID = 0? message.h #define ID_BROADCAST 0
    uint32_t readerID = 0;

    ret = new HeartbeatFrag(config.guidPrefix, readerID, config.writerUuid,
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
    logInfo("[Writer] checkSampleLiveliness")
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
        if(!change->isValid(this->config.deadline))
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




/***************************/
/* miscellaneous functions */ 
/***************************/



} //end namespace