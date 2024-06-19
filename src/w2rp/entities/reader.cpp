/*
 *
 */

#include <w2rp/entities/reader.hpp>
#include <stdexcept>

namespace w2rp {

Reader::Reader()
{
    logInfo("[Reader] empty constructor call")
}

Reader::Reader(uint16_t participant_id, config::readerCfg &cfg)
:
    config(cfg)
{
    init(participant_id);
    logInfo("[Reader] finished constructor call")
}

Reader::~Reader()
{
    delete netParser;
}

void Reader::init(uint16_t participant_id)
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

    writerProxy = new WriterProxy(this->config.sizeCache());

    // init net message parser
    netParser = new NetMessageParser();

    // init UDPComm object
    socket_endpoint rx_socketEndpoint = config.endpoint();
    socket_endpoint tx_socketEndpoint  = config.writer();

    logDebug("Reader: " << rx_socketEndpoint.ip_addr << ":" << rx_socketEndpoint.port)
    logDebug("Writer: " << tx_socketEndpoint.ip_addr << ":" << tx_socketEndpoint.port)


    CommInterface = new UDPComm(rx_socketEndpoint, tx_socketEndpoint);

    // create receive and receive handler threads
    recvThread = std::thread{&Reader::receiveMsg, this};
    handlerThread = std::thread{&Reader::handleMsg, this};

    this->nackCount = 0;

    this->debugCnt = 0;

    initialized = true;

    logInfo("[Reader] finished initialization")
}

void Reader::init(uint16_t participant_id, config::readerCfg &cfg)
{
    setConfig(cfg);
    init(participant_id);
}

/********************************************/
/** Callbacks triggered by external events **/ 
/********************************************/

void Reader::receiveMsg()
{      
    // msg to store received data
    MessageNet_t msg;

    while(true)
    {      
        CommInterface->receiveMsg(msg);
        receiveQueue.enqueue(msg);
        // logDebug("[Reader] received and enqueued message")
    }
}


void Reader::handleMsg()
{   
    // msg to store received data
    MessageNet_t msg;

    while(true)
    {              
        msg = receiveQueue.dequeue();
        // logDebug("[Reader] read msg from queue")
        
        handleMessages(&msg);
    }
}

bool Reader::handleMessages(MessageNet_t *net)
{
    // // debug only: test nackfrag, retx and timeout
    // if((debugCnt == 3))//|| (debugCnt == 2))
    // {
    //     debugCnt++;
    //     logDebug("[Reader] lost message")
    //     return false;
    // }
    // debugCnt++;
    // TODO remove previous block


    // logDebug("[Reader] handleMessages")
    // first extract submessages from msg
    std::vector<SubmessageBase*> res;

    netParser->getSubmessages(net, &res);
    
    // call corresponding submsg handler functions
    DataFrag* dataFrag;
    HeartbeatFrag* hbFrag;
    for(auto subMsg : res)
    {
        // logDebug("[Reader] handle submessage " << subMsg->subMsgHeader->submessageId)
        switch (subMsg->subMsgHeader->submessageId)
        {
        case DATA_FRAG:
            dataFrag = (DataFrag*)(subMsg);
            handleDataFrag(dataFrag);
            break;
        case HEARTBEAT_FRAG:
            hbFrag = (HeartbeatFrag*)(subMsg);
            handleHBFrag(hbFrag);
            break;
        default:
            break;
        }
    }

    // delete dataFrag;
    // delete hbFrag;
    // delete net;
    return true;
}

bool Reader::handleDataFrag(DataFrag *msg)
{
    // DataFrag received, update cache
    // logDebug("[Reader] handle  DataFrag")

    // TODO check for matching ReaderID??

    // first check liveliness of previous samples
    checkSampleLiveliness();

    // create new change for temporary usage
    auto change = new CacheChange(msg->writerSN, msg->dataSize, msg->fragmentSize, msg->timestamp);
    writerProxy->addChange(*change); // only adds change if new, else WriterProxy does nothing here

    // mark fragment as received 
    writerProxy->updateFragmentStatus(RECEIVED, msg->writerSN, msg->fragmentStartingNum, msg->serializedPayload, msg->fragmentSize); // msg->TODO also set data

    bool complete = writerProxy->checkSampleCompleteness(change->sequenceNumber);

    // if sample complete, send data up to application
    if(complete)
    {
        // create serializedPayload some sf data
        auto cfw  = writerProxy->getChange(change->sequenceNumber);
        SerializedPayload sampleData;
        buildSerializedSample(cfw, sampleData);

        // push to sampleQueue (application)
        sampleQueue.enqueue(sampleData);
        
        // logDebug("[Reader] sample complete: " << sampleData.data << "\n----------------------------------------------------------------------------------------------")

        // TODO remove sample from history?
    }    

    delete change;

    return true;
}

bool Reader::handleHBFrag(HeartbeatFrag *msg)
{
    
    // logDebug("[Reader] handle HBFrag")
    auto change = writerProxy->getChange(msg->writerSN);

    uint32_t lastFragmentNum = msg->lastFragmentNum;

    std::vector<uint32_t> missingFragments;

    uint32_t bitmapBase = 0;
    if(msg->lastFragmentNum > 256)
    {
        bitmapBase = msg->lastFragmentNum - 256;
    }

    change->getMissingFragments(lastFragmentNum, &missingFragments);

    unsigned char bitmap[8] = {0};

    if(missingFragments.size() > 0)
    {
        for(auto fn: missingFragments)
        {
            // Calculate the index in the bitmap array
            uint32_t index = (fn - bitmapBase) / 8;
            // Calculate the bit position within the byte
            uint32_t bitPosition = 7 - (fn - bitmapBase) % 8;
            // Set the bit at the calculated index and position
            bitmap[index] |= (1 << bitPosition);
        }
    }
    // else sent back an empty bitmap    

    // create W2RP header
    W2RPHeader *header = new W2RPHeader(guid.prefix);

    // logDebug("[Reader] Created W2RP Header: " << header->guidPrefix)


    // create NackFrag submessage
    // TODO writerID?
    uint32_t writerID = 0;
    NackFrag *response = new NackFrag(c_entityID_reader.to_uint32(), c_entityID_writer.to_uint32(),
                                        msg->writerSN, bitmapBase, bitmap, lastFragmentNum); // this->nackCount replaced with lastFragmentNum
    this->nackCount++;

    // serialization (toNet) and concatenation of submessages 
    MessageNet_t *txMsg = new MessageNet_t;
    header->headerToNet(txMsg);
    response->nackToNet(txMsg);

    // send message via UDP
    // logDebug("[Reader] Sending NackFrag")
    CommInterface->sendMsg(*txMsg);
    

    // delete objects
    // delete header;
    // delete response;
    // delete txMsg;

    return true;
}


/**************/
/* public API */
/**************/

 void Reader::retrieveSample(SerializedPayload &sample)
 {    
    if(initialized)
    {
        sample = sampleQueue.dequeue();        
    }
    else
    {
        logError("Reader is uninitialized, no config available")
        throw std::invalid_argument("initialization missing, config not found");
    }

    // logDebug("[Reader] sampleQueue dequeue - length:" << sample.length << " data: " << sample.data);
 }




/****************************************************/
/* methods used during hanlding of Data and HBFrags */
/****************************************************/



void Reader::buildSerializedSample(ChangeForWriter *cfw, SerializedPayload &sampleData)
{
    unsigned char* data = new unsigned char[cfw->sampleSize]{0};

    uint32_t pos = 0;

    auto fragmentArray = cfw->getFragmentArray();
    for(uint32_t i = 0; i < cfw->numberFragments; i++)
    {
        auto sf = fragmentArray[i];
        memcpy(data + pos, sf->data, sf->dataSize);
        pos += sf->dataSize;
    }

    sampleData = SerializedPayload(data, cfw->sampleSize);
    // TODO some stuff at the end of data
}


/*************************************************/
/* methods for checking validity of cacheChanges */ 
/*************************************************/


void Reader::checkSampleLiveliness()
{
    // TODO maybe, rather only remove samples once cache is full?
    if(writerProxy->getCurrentChange() == nullptr)
    {
        return;
    }

    std::vector<unsigned int> deprecatedSNs;
    // check liveliness of samples in history cache, if deadline expired remove sample from cache and ReaderProxies
    // assumption: samples transmitted in the right order and each sample has the same deadline, hence if the first
    // sample is not expired yet, all other example did not expire too
    while(1)
    {
        auto change = writerProxy->getCurrentChange();

        if(!change->isValid(this->config.deadline()))
        {
            deprecatedSNs.push_back(change->sequenceNumber);
            writerProxy->removeChange(change->sequenceNumber);
            if(writerProxy->getSize() == 0)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

/***************************/
/* miscellaneous functions */ 
/***************************/

void Reader::setConfig(config::readerCfg &cfg)
{
    config = cfg;
}


} //end namespace
