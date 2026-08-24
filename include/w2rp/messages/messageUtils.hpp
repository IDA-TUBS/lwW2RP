
#ifndef W2RP_MESSAGE_UTILS_H
#define W2RP_MESSAGE_UTILS_H

#include <vector>
#include <variant>

#include <w2rp/messages/message_net.hpp>
#include <w2rp/messages/messages.hpp>

namespace w2rp {

using SubmessageType = std::variant<
    DataFrag,
    HeartbeatFrag,
    NackFrag
>;

namespace messageUtils
{
    /**
     * @brief Builds a W2RP data message.
     * 
     * @param msg network message to be built
     * @param header W2RP header information
     * @param dataFrag Data fragment information
     * @param hbFrag Heartbeat fragment information (optional)
     */
    void buildDataMessage(
        MessageNet_t* msg, 
        W2RPHeader* header, 
        DataFrag* dataFrag, 
        HeartbeatFrag* hbFrag = nullptr
    );

    /**
     * @brief Extracts submessages from a W2RP message.
     * 
     * @param msg network message to be parsed
     * @return std::vector<SubmessageType> vector of extracted submessages
     */
    std::vector<SubmessageType> getSubmessages(MessageNet_t* msg);
   
} // end namespace messageUtils

} // end namespace w2rp

#endif // W2RP_MESSAGE_UTILS_H