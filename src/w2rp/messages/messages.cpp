/*
 *
 */

#include <w2rp/messages/messages.hpp>

namespace w2rp {

void W2RPHeader::headerToNet(MessageNet_t* msg)
{
    msg->add(&protocol, sizeof(protocol));
    msg->add(&version, sizeof(version));
    msg->add(&vendorID, sizeof(vendorID));
    msg->add(guidPrefix, 12);
}

void W2RPHeader::netToHeader(MessageNet_t* msg)
{
    msg->read(&protocol, sizeof(protocol));
    msg->read(&version, sizeof(version));
    msg->read(&vendorID, sizeof(vendorID));
    msg->read(guidPrefix, 12);
}





void SubmessageHeader::headerToNet(MessageNet_t* msg)
{
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
    msg->add(&is_last, sizeof(is_last));
}





void DataFrag::dataToNet(MessageNet_t* msg)
{
    // add submsg header
    subMsgHeader->headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&fragmentStartingNum, sizeof(fragmentStartingNum));
    msg->add(&fragmentsInSubmessage, sizeof(fragmentsInSubmessage));
    msg->add(&dataSize, sizeof(dataSize));
    msg->add(&fragmentSize, sizeof(fragmentSize));
    msg->add(serializedPayload, sizeof(fragmentSize));
    msg->add(&timestamp, sizeof(timestamp));
}

void DataFrag::netToData(MessageNet_t* msg)
{
    // read submsg header
    subMsgHeader->netToHeader(msg);
    
    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSN, sizeof(writerSN));
    msg->read(&writerSN, sizeof(writerSN));
    msg->read(&fragmentStartingNum, sizeof(fragmentStartingNum));
    msg->read(&fragmentsInSubmessage, sizeof(fragmentsInSubmessage));
    msg->read(&dataSize, sizeof(dataSize));
    msg->read(&fragmentSize, sizeof(fragmentSize));
    msg->read(serializedPayload, sizeof(fragmentSize));
    msg->read(&timestamp, sizeof(timestamp));
}





void NackFrag::nackToNet(MessageNet_t* msg)
{
    // add submsg header
    subMsgHeader->headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&fragmentNumberState, sizeof(fragmentNumberState));
    msg->add(&count, sizeof(count));
}

void NackFrag::netToNack(MessageNet_t* msg)
{
    // read submsg header
    subMsgHeader->netToHeader(msg);

    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSN, sizeof(writerSN));
    msg->read(&fragmentNumberState, sizeof(fragmentNumberState));
    msg->read(&count, sizeof(count));
}







void HeartbeatFrag::hbToNet(MessageNet_t* msg)
{
    // add submsg header
    subMsgHeader->headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&lastFragmentNum, sizeof(lastFragmentNum));
    msg->add(&count, sizeof(count));
}

void HeartbeatFrag::netToHB(MessageNet_t* msg)
{
    // read submsg header
    subMsgHeader->netToHeader(msg);

    // read contents
    msg->read(&readerID, sizeof(readerID));
    msg->read(&writerID, sizeof(writerID));
    msg->read(&writerSN, sizeof(writerSN));
    msg->read(&lastFragmentNum, sizeof(lastFragmentNum));
    msg->read(&count, sizeof(count));
}





} // end namespace