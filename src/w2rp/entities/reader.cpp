/*
 *
 */

#include <w2rp/entities/reader.h>

namespace w2rp {

Reader::Reader()
{
    // TODo fill reader config

    writerProxy = new WriterProxy(this->config.sizeCache);

    this->nackCount = 0;
}


Reader::~Reader()
{
    // TODO
}

bool Reader::handleMessages()
{
    // TODO
    return false;
}

bool Reader::handleDataFrag(DataFrag *msg)
{
    // DataFrag received, update cache

    // TODO use ReaderID??

    // first check liveliness of previous samples
    checkSampleLiveliness();

    // create new change
    auto change = new CacheChange(msg->writerSN, msg->dataSize, msg->fragmentSize, msg->timestamp);
    writerProxy->addChange(*change); // only adds change if new, else WriterProxy does nothing here

    // mark fragment as received 
    writerProxy->updateFragmentStatus(RECEIVED, msg->writerSN, msg->fragmentStartingNum, msg->serializedPayload, msg->fragmentSize); // msg->TODO also set data

    bool complete = writerProxy->checkSampleCompleteness(change->sequenceNumber);

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

    // TODO get guid prefix and writerID from somewhere?!
    unsigned char *guidPrefix;
    uint32_t writerID = 0;

    NackFrag *response = new NackFrag(guidPrefix, config.readerUuid, writerID,
                                        msg->writerSN, bitmapBase, bitmap, this->nackCount);
    this->nackCount++;

    // transmit Message ...

    return true;
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
