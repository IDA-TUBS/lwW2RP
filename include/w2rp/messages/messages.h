#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstring>
#include <chrono>


#ifndef MAX_MSG_LENGTH
#define MAX_MSG_LENGTH = 1472
#endif

namespace w2rp {

} //end namespace

enum SUBMESSAGES
{
    // basic RTPS submessages needed for W2RP
    NACK_FRAG = 0x12,
    HEARTBEAT_FRAG = 0x13,
    DATA_FRAG = 0x16,   
    
    // miscellaneous new submessages required for (some) W2RP extensions
    UPDATE = 0x20
};

struct FragmentNumberSet {
    uint32_t bitmapBase;
    unsigned char bitmap[8];
    uint32_t size = 4 + 8;
};



class W2RPHeader
{
  public:
    /*
     * @brief default constructor
     *
     * @param 12 byte guid prefix
     */
    W2RPHeader(unsigned char *guidPrefix)
    {
        this->protocol = this->encode("W2RP");
        this->version = 1;
        this->vendorID = 2;

        if (strlen((char*)guidPrefix) == 12) {
            // Copy the contents of guidPrefix into this->pre
            memcpy(this->guidPrefix, guidPrefix, 12);
        }

        this->length = sizeof(this->protocol) + 
                        sizeof(this->version) + 
                        sizeof(this->vendorID) + 
                        sizeof(this->guidPrefix);
    };

    /*
     * @brief default empty destructor
     */
    ~W2RPHeader(){};

    // message contents
    uint32_t protocol;              // Identifies the message as an W2RP message.
    uint16_t version;               // Identifies the version of the W2RP protocol.
    uint16_t vendorID;              // Indicates the vendor that provides the implementation of the W2RP protocol.
    unsigned char guidPrefix[12];   // Defines a default prefix to use for all GUIDs that appear in the message.

    // misc information
    uint32_t length;    

    /*
     * @brief encode protocol name in uint32_t
     *
     * @param string containing 4 letter protocol name
     * @return protocol name encoded in uint32_t
     */
    uint32_t encode(const char* str) {
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            result <<= 8; // Shift the bits to make space for the next character
            result |= str[i]; // Add the ASCII value of the character to the result
        }
        return result;
    };
};



class SubmessageHeader
{
  public:
    SubmessageHeader(uint8_t subMsgId, uint32_t subMsgLength, bool isLast): 
        submessageId(subMsgId),
        submessageLength(0),
        flags(0),
        is_last(isLast)
    {
        this->length = sizeof(this->submessageId) + 
                        sizeof(this->submessageLength) + 
                        sizeof(this->flags) + 
                        sizeof(this->is_last);
    };

    ~SubmessageHeader()
    {};

    // contents
    uint8_t submessageId;
    uint32_t submessageLength;  // without header
    uint8_t flags;
    bool is_last;

    // misc information
    uint32_t length;
};



// TODO add timestamp - non RTPS compliant?
class DataFrag
{
  public:
    /*
     * default constructor
     */
    DataFrag(unsigned char* guidPrefix, uint32_t readerID, uint32_t writerID,
             uint64_t writerSN, uint32_t fragmentStartingNum, 
             uint32_t dataSize, uint16_t fragmentSize, unsigned char *payload, std::chrono::system_clock::time_point sampleTimestamp):
             readerID(readerID),
             writerID(writerID), 
             writerSN(writerSN),
             fragmentStartingNum(fragmentStartingNum),
             fragmentsInSubmessage(1),
             dataSize(dataSize),
             fragmentSize(fragmentSize),
             timestamp(sampleTimestamp)
    {
        memcpy(this->serializedPayload, payload, fragmentSize);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(fragmentStartingNum) +
                       sizeof(fragmentsInSubmessage) +
                       sizeof(timestamp) +
                       fragmentSize;

        SubmessageHeader(DATA_FRAG, this->length, false);
    };

    /*
     * default destructor
     */
    ~DataFrag()
    {
        delete subMsgHeader;
        delete serializedPayload;
    };

    // contents
    SubmessageHeader *subMsgHeader;
    uint32_t readerID;                  // Identifies the Reader entity that is being informed of the change to the data-object. // TODO require multicast readerID?!
    uint32_t writerID;                  // Identifies the Writer entity that made the change to the data- object.
    uint64_t writerSN;                  // Uniquely identifies the change and the relative order for all changes made by the Writer.
    uint32_t fragmentStartingNum;       // Indicates the starting fragment for the series of fragments in the serializedPayload field
    uint16_t fragmentsInSubmessage;     // The number of consecutive fragments contained in this Submessage
    uint32_t dataSize;                  // The total size in bytes of the original data before fragmentation.
    uint16_t fragmentSize;              // The size of an individual fragment in bytes. The maximum fragment size equals 64K.
    // parameterList inlineQos;
    unsigned char *serializedPayload;  // actual payload
    std::chrono::system_clock::time_point timestamp; // timestamp signaling the arrival of the sample at the writer, required at the reader for determining deadline violations. Requires time sync between nodes hosting writer and reader 

    // misc information
    uint32_t length;
};



class NackFrag
{
  public:
    /*
     * default constructor
     */
    NackFrag(unsigned char* guidPrefix, uint32_t readerID, uint32_t writerID,
             uint64_t writerSN, uint16_t bitmapBase, unsigned char *fragmentStates, uint32_t NackFragCount):
             readerID(readerID),
             writerID(writerID),
             writerSN(writerSN),
             count(NackFragCount)
    {
        memcpy(this->fragmentNumberState.bitmap, fragmentStates, 8);
        this->fragmentNumberState.bitmapBase = bitmapBase;

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(count) +
                       fragmentNumberState.size;

        SubmessageHeader(NACK_FRAG, this->length, false);
    };

    /*
     * default destructor
     */
    ~NackFrag()
    {
        delete subMsgHeader;
    };

    // contents
    SubmessageHeader *subMsgHeader;
    uint32_t readerID;                      // Identifies the Reader entity that requests to receive certain fragments.
    uint32_t writerID;                      // Identifies the Writer entity that is the target of the NackFrag message. This is the Writer Entity that is being asked to re-send some fragments.
    uint64_t writerSN;                      // The sequence number for which some fragments are missing.
    FragmentNumberSet fragmentNumberState;  // Communicates the state of the reader to the writer. The fragment numbers that appear in the set indicate missing fragments on the reader side. The ones that do not appear in the set are undetermined (could have been received or not).
    uint32_t count;                         // Reader's NackFrag counter // repurposed for returning the HB's lastFragmentNum
    
    // misc information
    uint32_t length;
};


class HeartbeatFrag
{
  public:
    /*
     * default constructor
     */
    HeartbeatFrag(unsigned char* guidPrefix, uint32_t readerID, uint32_t writerID,
             uint64_t writerSN, uint32_t lastFragmentNum, uint32_t HBFragCount):
             readerID(readerID),
             writerID(writerID),
             writerSN(writerSN),
             lastFragmentNum(lastFragmentNum),
             count(HBFragCount)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(lastFragmentNum) + 
                       sizeof(count);
        SubmessageHeader(HEARTBEAT_FRAG, this->length, false);
    };

    /*
     * default destructor
     */
    ~HeartbeatFrag()
    {
        delete subMsgHeader;
    };

    // contents
    SubmessageHeader *subMsgHeader;
    uint32_t readerID;          // Identifies the Reader Entity that is being informed of the availability of fragments. Can be set to ENTITYID_UNKNOWN to indicate all readers for the writer that sent the message.
    uint32_t writerID;          // Identifies the Writer Entity that sent the Submessage.
    uint64_t writerSN;          // Identifies the sequence number of the data change for which fragments are available.
    uint32_t lastFragmentNum;   // All fragments up to and including this last (highest) fragment are available on the Writer for the change identified by writerSN.
    uint32_t count;   

    // misc information
    uint32_t length;
};


#endif //MESSAGES_H