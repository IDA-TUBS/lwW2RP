#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstring>
#include <chrono>
#include <vector>
#include <w2rp/messages/message_net.hpp>
#include <w2rp/config/config.hpp>
#include <w2rp/log.hpp>
#include <w2rp/guid/guid.hpp>
#include <w2rp/guid/guidPrefix.hpp>


namespace w2rp {

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
    unsigned char bitmap[NACK_BITMAP_SIZE];
    uint32_t size = 4 + NACK_BITMAP_SIZE;
};


// TODO add timestamp - non RTPS compliant?
class W2RPHeader
{
  public:
    /**
     * @brief constructor for empty submessage object
     */
    W2RPHeader()
    {
        this->length = sizeof(this->protocol) + 
                        sizeof(this->version) + 
                        sizeof(this->vendorID) + 
                        sizeof(this->guidPrefix);
    };


    /**
     * @brief constructor
     *
     * @param 12 byte guid prefix
     */
    W2RPHeader(GuidPrefix_t &prefix)
    {
        this->protocol = this->encode("W2RP");
        this->version = 1;
        this->vendorID = vendorID_IDA;

        guidPrefix = prefix;

        this->length = sizeof(this->protocol) + 
                        sizeof(this->version) + 
                        sizeof(this->vendorID) + 
                        sizeof(this->guidPrefix);
    };

    /**
     * @brief copy constructor
     */
    W2RPHeader(W2RPHeader &header)
    {
        this->protocol = header.protocol;
        this->version = header.version;
        this->vendorID = header.vendorID;
        this->guidPrefix = header.guidPrefix;

        this->length = header.length;
    };

    /**
     * @brief default empty destructor
     */
    ~W2RPHeader(){};

    // message contents
    uint32_t protocol;              // Identifies the message as an W2RP message.
    uint16_t version;               // Identifies the version of the W2RP protocol.
    uint16_t vendorID;              // Indicates the vendor that provides the implementation of the W2RP protocol.
    GuidPrefix_t guidPrefix;   // Defines a default prefix to use for all GUIDs that appear in the message.

    // misc information
    uint32_t length;    

    /**
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


    /**
     * @brief Convert header to char array
     * 
     * @param msg char array to store the byte stream
     */
    void headerToNet(MessageNet_t* msg);

    /**
     * @brief Convert char array to header
     * 
     * @param msg char array to store the byte stream
     */
    void netToHeader(MessageNet_t* msg);

};



class SubmessageHeader
{
  public:
    /**
     * @brief constructor for empty submessage object
     */
    SubmessageHeader()
    {
        this->length = sizeof(this->submessageId) + 
                        sizeof(this->submessageLength) + 
                        sizeof(this->flags) + 
                        sizeof(this->is_last);
    };

    /**
     * @brief constructor
     *
     * @param id of submessage
     * @param length of submessage
     * @param true if last submsg, else false
     */
    SubmessageHeader(uint8_t subMsgId, uint32_t subMsgLength, bool isLast): 
        submessageId(subMsgId),
        submessageLength(subMsgLength),
        flags(0),
        is_last(isLast)
    {
        this->length = sizeof(this->submessageId) + 
                        sizeof(this->submessageLength) + 
                        sizeof(this->flags) + 
                        sizeof(this->is_last);
    };

    /**
     * @brief copy constructor
     */
    SubmessageHeader(SubmessageHeader &header)
    {
        this->submessageId = header.submessageId;
        this->submessageLength = header.submessageLength;
        this->flags = header.flags;
        this->is_last = header.is_last;
        this->length = header.length;
    };

    /**
     * @brief destructor
     */
    ~SubmessageHeader()
    {};

    // contents
    uint8_t submessageId;
    uint32_t submessageLength;  // without header
    uint8_t flags;
    bool is_last;

    // misc information
    uint32_t length;

    /**
     * @brief Convert submsg header to char array
     * 
     * @param msg char array to store the byte stream
     */
    void headerToNet(MessageNet_t* msg);

    /**
     * @brief Convert char array to submsg header
     * 
     * @param msg char array to store the byte stream
     */
    void netToHeader(MessageNet_t* msg);
};

class SubmessageBase
{
  public:
    /**
     * @brief constructor for base msg object
     */
    SubmessageBase()
    {};

    /**
     * @brief destructor
     */
    ~SubmessageBase()
    {
        if(subMsgHeader)
        {
            delete subMsgHeader;
        }
    };

    // contents
    SubmessageHeader *subMsgHeader = nullptr;

};


class DataFrag: public SubmessageBase
{
  public:
    /**
     * @brief constructor for empty submessage object
     */
    DataFrag()
    {
        subMsgHeader = new SubmessageHeader(DATA_FRAG, this->length, false);
    };

    /**
     * @brief constructor
     */
    DataFrag(uint32_t readerID, uint32_t writerID,
             uint64_t writerSN, uint32_t fragmentStartingNum, 
             uint32_t dataSize, uint16_t fragmentSize, unsigned char *payload, std::chrono::system_clock::time_point sampleTimestamp
    ):
             readerID(readerID),
             writerID(writerID), 
             writerSN(writerSN),
             fragmentStartingNum(fragmentStartingNum),
             fragmentsInSubmessage(1),
             dataSize(dataSize),
             fragmentSize(fragmentSize),
             timestamp(sampleTimestamp)
    {
        this->serializedPayload = new unsigned char[fragmentSize]{0};
        memset(this->serializedPayload, 0, fragmentSize * sizeof(unsigned char));
        memcpy(this->serializedPayload, payload, fragmentSize);

        subMsgHeader = new SubmessageHeader(DATA_FRAG, this->length, false);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(fragmentStartingNum) +
                       sizeof(fragmentsInSubmessage) +
                       sizeof(timestamp) +
                       fragmentSize;
    };

    /**
     * @brief copy constructor
     */
    DataFrag(DataFrag &frag)
    {
        subMsgHeader = frag.subMsgHeader;

        this->readerID = frag.readerID;
        this->writerID = frag.writerID;
        this->writerSN = frag.writerSN;
        this->fragmentStartingNum = frag.fragmentStartingNum;
        this->fragmentsInSubmessage = frag.fragmentsInSubmessage;
        this->dataSize = frag.dataSize;
        this->fragmentSize = frag.fragmentSize;

        this->serializedPayload = new unsigned char[fragmentSize]{0};
        memset(this->serializedPayload, 0, fragmentSize * sizeof(unsigned char));
        memcpy(this->serializedPayload, frag.serializedPayload, frag.fragmentSize);
        this->timestamp = frag.timestamp;

        this->length = frag.length;
    };

    /**
     * @brief default destructor
     */
    ~DataFrag()
    {
        if(serializedPayload)
        {
            delete serializedPayload;
        }
    };

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

    /**
     * @brief Convert data frag to char array
     * 
     * @param msg char array to store the byte stream
     */
    void dataToNet(MessageNet_t* msg);

    /**
     * @brief Convert char array to data frag
     * 
     * @param msg char array to store the byte stream
     */
    void netToData(MessageNet_t* msg);

    void print();
};



class NackFrag: public SubmessageBase
{
  public:
    /**
    * @brief constructor for empty submessage object
    */
    NackFrag()
    {
        // subMsgHeader = new SubmessageHeader(NACK_FRAG, this->length, false);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(count) +
                       fragmentNumberState.size;

        subMsgHeader = new SubmessageHeader(NACK_FRAG, this->length, false);
    };

    /** 
     * @brief constructor
     */
    NackFrag(uint32_t readerID, uint32_t writerID,
             uint64_t writerSN, uint16_t bitmapBase, unsigned char *fragmentStates, uint32_t NackFragCount):
             readerID(readerID),
             writerID(writerID),
             writerSN(writerSN),
             count(NackFragCount)
    {
        memcpy(this->fragmentNumberState.bitmap, fragmentStates, NACK_BITMAP_SIZE);
        this->fragmentNumberState.bitmapBase = bitmapBase;

        // subMsgHeader = new SubmessageHeader(NACK_FRAG, this->length, false);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(count) +
                       fragmentNumberState.size;

        subMsgHeader = new SubmessageHeader(NACK_FRAG, this->length, false);
    };

    /**
     * @brief copy constructor
     */
    NackFrag(NackFrag &nack)
    {
        subMsgHeader = nack.subMsgHeader;
        
        this->readerID = nack.readerID;
        this->writerID = nack.writerID;
        this->writerSN = nack.writerSN;
        this->fragmentNumberState = nack.fragmentNumberState;
        this->count = nack.count;
        

        this->length = nack.length;
    };

    /**
     * @brief default destructor
     */
    ~NackFrag()
    {};

    // contents
    uint32_t readerID;                      // Identifies the Reader entity that requests to receive certain fragments.
    uint32_t writerID;                      // Identifies the Writer entity that is the target of the NackFrag message. This is the Writer Entity that is being asked to re-send some fragments.
    uint64_t writerSN;                      // The sequence number for which some fragments are missing.
    FragmentNumberSet fragmentNumberState;  // Communicates the state of the reader to the writer. The fragment numbers that appear in the set indicate missing fragments on the reader side. The ones that do not appear in the set are undetermined (could have been received or not).
    uint32_t count;                         // Reader's NackFrag counter // repurposed for returning the HB's lastFragmentNum
    
    // misc information
    uint32_t length;

    /**
     * @brief Convert nack frag to char array
     * 
     * @param msg char array to store the byte stream
     */
    void nackToNet(MessageNet_t* msg);

    /**
     * @brief Convert char array to nack frag
     * 
     * @param msg char array to store the byte stream
     */
    void netToNack(MessageNet_t* msg);
};


class HeartbeatFrag: public SubmessageBase
{
  public:
    /**
    * @brief constructor for empty submessage object
    */
    HeartbeatFrag()
    {
        subMsgHeader = new SubmessageHeader(HEARTBEAT_FRAG, this->length, false);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(lastFragmentNum) + 
                       sizeof(count);
    };

    /**
     * @brief default constructor
     */
    HeartbeatFrag(uint32_t readerID, uint32_t writerID,
                uint64_t writerSN, uint32_t lastFragmentNum, uint32_t HBFragCount):
             readerID(readerID),
             writerID(writerID),
             writerSN(writerSN),
             lastFragmentNum(lastFragmentNum),
             count(HBFragCount)
    {
        subMsgHeader = new SubmessageHeader(HEARTBEAT_FRAG, this->length, false);

        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSN) +
                       sizeof(lastFragmentNum) + 
                       sizeof(count);
    };

    /**
     * @brief copy constructor
     */
    HeartbeatFrag(HeartbeatFrag &hb)
    {
        subMsgHeader = hb.subMsgHeader;
        
        this->readerID = hb.readerID;
        this->writerID = hb.writerID;
        this->writerSN = hb.writerSN;
        this->lastFragmentNum = hb.lastFragmentNum;
        this->count = hb.count;
        

        this->length = hb.length;
    };

    /**
     * @brief default destructor
     */
    ~HeartbeatFrag()
    {};

    // contents
    uint32_t readerID;          // Identifies the Reader Entity that is being informed of the availability of fragments. Can be set to ENTITYID_UNKNOWN to indicate all readers for the writer that sent the message.
    uint32_t writerID;          // Identifies the Writer Entity that sent the Submessage.
    uint64_t writerSN;          // Identifies the sequence number of the data change for which fragments are available.
    uint32_t lastFragmentNum;   // All fragments up to and including this last (highest) fragment are available on the Writer for the change identified by writerSN.
    uint32_t count;   

    // misc information
    uint32_t length;


    /**
     * @brief Convert hb frag to char array
     * 
     * @param msg char array to store the byte stream
     */
    void hbToNet(MessageNet_t* msg);

    /**
     * @brief Convert char array to hb frag
     * 
     * @param msg char array to store the byte stream
     */
    void netToHB(MessageNet_t* msg);
};



class NetMessageParser
{
  public:
    /**
    * @brief constructor
    */
    NetMessageParser()
    {};

    /**
    * @brief destructor
    */
    ~NetMessageParser()
    {};


    void getSubmessages(MessageNet_t* msg, std::vector<SubmessageBase*> *res);
};


} //end namespace

#endif //MESSAGES_H