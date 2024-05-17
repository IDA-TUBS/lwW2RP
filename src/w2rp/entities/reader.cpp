/*
 *
 */

#include <w2rp/entities/reader.hpp>

namespace w2rp {

Reader::Reader()
{
    // TODo fill reader config

    writerProxy = new WriterProxy(this->config.sizeCache);

    // init net message parser
    netParser = new NetMessageParser();

    // init UDPComm object
    socket_endpoint rx_socketEndpoint(config.writerAddress, config.writerPort);
    socket_endpoint tx_socketEndpoint(config.readerAddress, config.readerPort);
    CommInterface = new UDPComm(rx_socketEndpoint, tx_socketEndpoint);

    // create receive and receive handler threads
    recvThread = std::thread{&Reader::receiveMsg, this};
    handlerThread = std::thread{&Reader::handleMsg, this};

    this->nackCount = 0;
}


Reader::~Reader()
{
    delete netParser;
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
    }
}


void Reader::handleMsg()
{    
    // msg to store received data
    MessageNet_t msg;

    while(true)
    {              
        msg = receiveQueue.dequeue();
        
        handleMessages(&msg);
    }
}

bool Reader::handleMessages(MessageNet_t *net)
{
    // first extract submessages from msg
    std::vector<SubmessageBase*> res;

    netParser->getSubmessages(net, &res);
    
    // call corresponding submsg handler functions
    DataFrag* dataFrag;
    HeartbeatFrag* hbFrag;
    for(auto subMsg : res)
    {
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

    delete dataFrag;
    delete hbFrag;
    delete net;
    return true;
}

bool Reader::handleDataFrag(DataFrag *msg)
{
    // DataFrag received, update cache

    // TODO check for matching ReaderID??

    // first check liveliness of previous samples
    checkSampleLiveliness();

    // create new change
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
    }    

    delete change;

    return true;
}

bool Reader::handleHBFrag(HeartbeatFrag *msg)
{
    auto change = writerProxy->getChange(msg->writerSN);

    uint32_t lastFragmentNum = msg->lastFragmentNum;

    std::vector<uint32_t> *missingFragments;

    uint32_t bitmapBase = 0;
    if(msg->lastFragmentNum > 256)
    {
        bitmapBase = msg->lastFragmentNum - 256;
    }

    change->getMissingFragments(lastFragmentNum, missingFragments);

    unsigned char bitmap[8] = {0};

    for(auto fn: *missingFragments)
    {
        // Calculate the index in the bitmap array
        uint32_t index = (fn - bitmapBase) / 8;
        // Calculate the bit position within the byte
        uint32_t bitPosition = (fn - bitmapBase) % 8;
        // Set the bit at the calculated index and position
        bitmap[index] |= (1 << bitPosition);
    }

    // create W2RP header
    W2RPHeader *header = new W2RPHeader(config.guidPrefix);

    // create NackFrag submessage
    // TODO writerID?
    uint32_t writerID = 0;
    NackFrag *response = new NackFrag(config.guidPrefix, config.readerUuid, writerID,
                                        msg->writerSN, bitmapBase, bitmap, lastFragmentNum); // this->nackCount replaced with lastFragmentNum
    this->nackCount++;


    // serialization (toNet) and concatenation of submessages 
    MessageNet_t *txMsg;
    header->headerToNet(txMsg);
    response->nackToNet(txMsg);

    // send message via UDP
    CommInterface->sendMsg(*txMsg);
    

    // delete object
    delete header;
    delete response;
    delete txMsg;

    return true;
}


/**************/
/* public API */
/**************/

 void Reader::retrieveSample(SerializedPayload &data)
 {
    // msg to store received data
    SerializedPayload sample;

    while(true)
    {              
        sample = sampleQueue.dequeue();
        
        data = sample;
    }
 }




/****************************************************/
/* methods used during hanlding of Data and HBFrags */
/****************************************************/



void Reader::buildSerializedSample(ChangeForWriter *cfw, SerializedPayload &sampleData)
{
    unsigned char data[cfw->sampleSize];

    uint32_t pos = 0;

    auto fragmentArray = cfw->getFragmentArray();
    for(uint32_t i = 0; i < cfw->numberFragments; i++)
    {
        auto sf = fragmentArray[i];
        memcpy(data + pos, sf->data, sf->dataSize);
        pos += sf->dataSize;
    }

    SerializedPayload payload(data, cfw->sampleSize);

    sampleData = payload;
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

        if(!change->isValid(this->config.deadline))
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


} //end namespace
