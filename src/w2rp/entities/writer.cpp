/*
 *
 */

#include <w2rp/entities/writer.h>

namespace w2rp {

Writer::Writer()
{
    // TODO fill writer config


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
}

Writer::~Writer()
{
    matchedReaders.clear();
    historyCache.clear();
    sendQueue.clear();
}

/********************************************
 ** Callbacks triggered by external events ** 
********************************************/

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
    // create CacheChange object
    CacheChange *newChange = new CacheChange(sequenceNumberCnt++, data->length, config.fragmentSize, timestamp);

    // fragment sample
    std::vector<SampleFragment*> *fragments;
    bool compare = false;
    sampleFragmenter->fragmentPayload(data, newChange, fragments, timestamp, compare);
    newChange->setFragmentArray(fragments);


    // add CacheChange to history
    if(historyCache.size() == this->config.sizeCache)
    {
        return false;
    }
    historyCache.push_back(newChange);
    

    // generate ChangeForReaders based on CacheChange and add to reader proxies (done by ReaderProxy itself)
    for (auto rp: matchedReaders)
    {
        rp->addChange(*newChange);
    }

    return true;
}





/*********************************************
 * methods used during fragment transmission * 
 *********************************************/










/*************************************************
 * methods for checking validity of cacheChanges * 
 ************************************************/

void Writer::checkSampleLiveliness()
{
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

    std::vector<unsigned int> deprecatedSNs;
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

    for (unsigned int sequenceNumber: deprecatedSNs)
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
            unsigned int sequenceNumber = (*it)->baseChange->sequenceNumber;
            for(unsigned int deprecatedSn: deprecatedSNs)
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

} //end namespace