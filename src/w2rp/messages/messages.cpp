/*
 *
 */

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
    // logDebug("[MESSAGE] DataFrag: dataToNet, pos: " << msg->pos)
    // add submsg header
    subMsgHeader->headerToNet(msg);

    // TODO make bool and return false in case of msg creation failed due to unknown size issues?

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&fragmentStartingNum, sizeof(fragmentStartingNum));
    msg->add(&fragmentsInSubmessage, sizeof(fragmentsInSubmessage));
    msg->add(&dataSize, sizeof(dataSize));
    msg->add(&fragmentSize, sizeof(fragmentSize));
    msg->add(serializedPayload, fragmentSize);
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
    // Allocate memory for the payload data
    this->serializedPayload = new unsigned char[this->fragmentSize];  
    msg->read(serializedPayload, fragmentSize);
    msg->read(&timestamp, sizeof(timestamp));
    
    this->length = sizeof(readerID) +
                sizeof(writerID) +
                sizeof(writerSN) +
                sizeof(fragmentStartingNum) +
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
    // logDebug("[MESSAGE] HeartbeatFrag: hbToNet, pos: " << msg->pos)
    // add submsg header
    subMsgHeader->headerToNet(msg);

    // add contents
    msg->add(&readerID, sizeof(readerID));
    msg->add(&writerID, sizeof(writerID));
    msg->add(&writerSN, sizeof(writerSN));
    msg->add(&lastFragmentNum, sizeof(lastFragmentNum));
    msg->add(&count, sizeof(count));
    // logDebug("[MESSAGE] HeartbeatFrag: hbToNet 2")
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








void NetMessageParser::getSubmessages(MessageNet_t* msg, std::vector<SubmessageBase*> *res)
{
    W2RPHeader *w2rpHeader;
    w2rpHeader = new W2RPHeader();

    SubmessageHeader *subMsgHeader;
    subMsgHeader = new SubmessageHeader();

    w2rpHeader->netToHeader(msg);
    // logDebug("[NetMessageParser] Parse packet of length: " << msg->length)

    while (true)
    {
        // check if end of message has been reached
        if(msg->pos >= (msg->length - 1))
        {
            // end of message reached
            // logDebug("[NetMessageParser] end of message reached, pos: " << msg->pos)
            break;
        }
        // logDebug("[NetMessageParser] Parse SubmessageHeader, pos: " << msg->pos)

        // parse submessage header
        subMsgHeader->netToHeader(msg);
        // logDebug("[NetMessageParser] SubmessageHeader, length: " << subMsgHeader->length)

        // based on id, parse corresponding submessage
        switch (subMsgHeader->submessageId)
        {
        case DATA_FRAG:
            if(msg->movePos(-(subMsgHeader->length)))
            {
                // logDebug("[NetMessageParser] Parse DataFrag, pos: " << msg->pos)
                DataFrag *dataFrag;
                dataFrag = new DataFrag();

                dataFrag->netToData(msg);

                res->push_back(dataFrag);
            }
            else
            {
                // something went wrong
                // logDebug("[NetMessageParser] something went wrong")
                // TODO handle error
            }
            break;
        case HEARTBEAT_FRAG:
            if(msg->movePos(-(subMsgHeader->length)))
            {
                // logDebug("[NetMessageParser] Parse HeartbeatFrag, pos: " << msg->pos)
                HeartbeatFrag *hbFrag;
                hbFrag = new HeartbeatFrag();

                hbFrag->netToHB(msg);

                res->push_back(hbFrag);
            }
            else
            {
                // something went wrong
                // logDebug("[NetMessageParser] something went wrong")
                // TODO handle error
            }
            break;
        case NACK_FRAG:
            if(msg->movePos(-(subMsgHeader->length)))
            {
                // logDebug("[NetMessageParser] Parse NackFrag, pos: " << msg->pos)
                NackFrag *nackFrag;
                nackFrag = new NackFrag();

                nackFrag->netToNack(msg);

                res->push_back(nackFrag);
            }
            else
            {
                // something went wrong
                // logDebug("[NetMessageParser] something went wrong")
                // TODO handle error
            }
            break;
        default:
            break;
        }
    }
    
}


} // end namespace