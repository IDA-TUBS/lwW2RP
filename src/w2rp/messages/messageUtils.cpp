#include <w2rp/messages/messageUtils.hpp>

namespace w2rp {

void messageUtils::buildDataMessage(
    MessageNet_t* msg, 
    W2RPHeader* header, 
    DataFrag* dataFrag, 
    HeartbeatFrag* hbFrag
)
{
    // clear message
    msg->clear();

    // add W2RP header
    header->headerToNet(msg);

    // add DataFrag submessage
    dataFrag->dataToNet(msg);

    // optionally add HeartbeatFrag submessage
    if(hbFrag != nullptr)
    {
        hbFrag->hbToNet(msg);
    }
}

std::vector<SubmessageType> messageUtils::getSubmessages(MessageNet_t* msg)
{
    std::vector<SubmessageType> submessageVector;
    SubmessageHeader subMsgHeader;

    // Move read head to submessage segment
    msg->reset();
    msg->movePos(W2RPHeader().length);

    while(msg->pos < msg->length)
    {
        // parse submessage header
        subMsgHeader.netToHeader(msg);

        // reset read head to start of submessage
        if(!msg->movePos(-(subMsgHeader.length)))
        {
            logError("[messageUtils][getSubmessages] resetting read head failed")
            break;
        }

        // based on id, parse corresponding submessage
        switch(subMsgHeader.submessageId)
        {
            case DATA_FRAG:
            {
                submessageVector.emplace_back(std::in_place_type<DataFrag>, msg);
                break;
            }
            case HEARTBEAT_FRAG:
            {
                submessageVector.emplace_back(std::in_place_type<HeartbeatFrag>, msg);
                break;
            }
            case NACK_FRAG:
            {
                submessageVector.emplace_back(std::in_place_type<NackFrag>, msg);
                break;
            }
            default:
                logError("[messageUtils][getSubmessages] Unknown sub-message")
                break;
        }

        // terminate if last submessage has been processed
        if(subMsgHeader.is_last)
        {
            break;
        }
    }
    return submessageVector;
}

} // end namespace w2rp