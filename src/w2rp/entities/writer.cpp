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

    // initialize readerProxies
}

Writer::~Writer()
{
    matchedReaders.clear();
    historyCache.clear();
    sendQueue.clear();
}

bool Writer::handleNewSample(SerializedPayload *data)
{
    auto timestamp_now = std::chrono::system_clock::now();

    // create CacheChange object
    CacheChange *newChange = new CacheChange(sequenceNumberCnt++, data->length, config.fragmentSize, timestamp_now);

    // fragment sample
    std::vector<SampleFragment*>* fragments;
    bool compare = false;
    sampleFragmenter->fragmentPayload(data, newChange, fragments, timestamp_now, compare);


    if(historyCache.size() == this->sizeCache)
    {
        return false;
    }
    historyCache.push_back(newChange);

    // add CacheChange to history

}

} //end namespace