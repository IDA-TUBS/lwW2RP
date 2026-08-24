#include <w2rp/messages/messages.hpp>


namespace w2rp {

void W2RPHeader::headerToNet(MessageNet_t* msg)
{
    // logDebug("[MESSAGE] W2RPHeader: headerToNet, pos: " << msg->pos)
    msg->add(&protocol, sizeof(protocol));
    msg->add(&version, sizeof(version));
    msg->add(&vendorID, sizeof(vendorID));
    msg->add(guidPrefix.value, guidPrefix.size);
}

void W2RPHeader::netToHeader(MessageNet_t* msg)
{
    msg->read(&protocol, sizeof(protocol));
    msg->read(&version, sizeof(version));
    msg->read(&vendorID, sizeof(vendorID));
    msg->read(guidPrefix.value, guidPrefix.size);
}

void SubmessageHeader::headerToNet(MessageNet_t* msg)
{   
    // logDebug("[MESSAGE] SubmessageHeader: headerToNet, pos: " << msg->pos)
    msg->add(&submessageId, sizeof(submessageId));
    msg->add(&submessageLength, sizeof(submessageLength));
    msg->add(&flags, sizeof(flags));
    msg->add(&is_last, sizeof(is_last));
}

void SubmessageHeader::netToHeader(MessageNet_t* msg)
{
    msg->read(&submessageId, sizeof(submessageId));
    msg->read(&submessageLength, sizeof(submessageLength));
    msg->read(&flags, sizeof(flags));
    msg->read(&is_last, sizeof(is_last));

    this->length = sizeof(this->submessageId) + 
                        sizeof(this->submessageLength) + 
                        sizeof(this->flags) + 
                        sizeof(this->is_last);
}

void DataFrag::dataToNet(MessageNet_t* msg)
{
    // add submsg header
    subMsgHeader.headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->add(&fragmentNumber, sizeof(fragmentNumber));
    msg->add(&fragmentsInSubmessage, sizeof(fragmentsInSubmessage));
    msg->add(&sampleSize, sizeof(sampleSize));
    msg->add(&fragmentSize, sizeof(fragmentSize));
    msg->add(serializedPayload.data(), fragmentSize);
    msg->add(&timestamp, sizeof(timestamp));
}

void DataFrag::netToData(MessageNet_t* msg)
{    
    // read submsg header
    subMsgHeader.netToHeader(msg);
    
    // reserve space for payload
    serializedPayload.resize(fragmentSize);

    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->read(&fragmentNumber, sizeof(fragmentNumber));
    msg->read(&fragmentsInSubmessage, sizeof(fragmentsInSubmessage));
    msg->read(&sampleSize, sizeof(sampleSize));
    msg->read(&fragmentSize, sizeof(fragmentSize));
    
    // reserve space for payload
    serializedPayload.resize(fragmentSize);
    
    msg->read(serializedPayload.data(), fragmentSize);
    msg->read(&timestamp, sizeof(timestamp));
    
    this->length = sizeof(readerID) +
                sizeof(writerID) +
                sizeof(writerSequenceNumber) +
                sizeof(fragmentNumber) +
                sizeof(fragmentsInSubmessage) +
                sizeof(timestamp) +
                fragmentSize;
}

void DataFrag::print()
{
    // logDebug("[DataFrag] readerID: " << readerID)
    // logDebug("[DataFrag] writerID: " << writerID)
}

void NackFrag::nackToNet(MessageNet_t* msg)
{
    // logDebug("[MESSAGE] NackFrag: nackToNet, pos: " << msg->pos)
    // add submsg header
    subMsgHeader.headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->add(&fragmentStatus, sizeof(fragmentStatus));
    msg->add(&count, sizeof(count));
}

void NackFrag::netToNack(MessageNet_t* msg)
{
    // read submsg header
    subMsgHeader.netToHeader(msg);

    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->read(&fragmentStatus, sizeof(fragmentStatus));
    msg->read(&count, sizeof(count));
}

void HeartbeatFrag::hbToNet(MessageNet_t* msg)
{
    // logDebug("[MESSAGE] HeartbeatFrag: hbToNet, pos: " << msg->pos)
    // add submsg header
    subMsgHeader.headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->add(&highestFragmentNum, sizeof(highestFragmentNum));
    msg->add(&count, sizeof(count));
    // logDebug("[MESSAGE] HeartbeatFrag: hbToNet 2")
}

void HeartbeatFrag::netToHB(MessageNet_t* msg)
{
    // read submsg header
    subMsgHeader.netToHeader(msg);

    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSequenceNumber, sizeof(writerSequenceNumber));
    msg->read(&highestFragmentNum, sizeof(highestFragmentNum));
    msg->read(&count, sizeof(count));
}

} // end namespace