#ifndef W2RP_MESSAGES_H
#define W2RP_MESSAGES_H

#include <cstring>
#include <chrono>
#include <vector>
#include <cstdint>
#include <variant>

#include <w2rp/messages/message_net.hpp>
#include <w2rp/config/config.hpp>
#include <w2rp/log.hpp>
#include <w2rp/guid/guid.hpp>
#include <w2rp/guid/guidPrefix.hpp>
#include <w2rp/cacheChange.hpp>
#include <w2rp/fragment/sampleFragment.hpp>
#include <w2rp/fragment/sampleFragmentState.hpp>



namespace w2rp {

enum SUBMESSAGES
{
    // basic RTPS submessages needed for W2RP
    NACK_FRAG = 0x12,
    HEARTBEAT_FRAG = 0x13,
    DATA_FRAG = 0x16,   
};

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
    std::uint32_t protocol;    // Identifies the message as an W2RP message.
    std::uint16_t version;     // Identifies the version of the W2RP protocol.
    std::uint16_t vendorID;    // Indicates the vendor that provides the implementation of the W2RP protocol.
    GuidPrefix_t guidPrefix;   // Defines a default prefix to use for all GUIDs that appear in the message.

    // misc information
    std::uint32_t length;    

    /**
     * @brief encode protocol name in std::uint32_t
     *
     * @param string containing 4 letter protocol name
     * @return protocol name encoded in std::uint32_t
     */
    std::uint32_t encode(const char* str) {
        std::uint32_t result = 0;
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
    SubmessageHeader(
        std::uint8_t subMsgId, 
        std::uint32_t subMsgLength, 
        bool isLast
    ): 
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
    SubmessageHeader(SubmessageHeader &header) = default;

    /**
     * @brief copy assignment operator
     */
    SubmessageHeader& operator=(const SubmessageHeader &header) = default; 

    /**
     * @brief move constructor
     */
    SubmessageHeader(SubmessageHeader&&) noexcept = default;

    /**
     * @brief move assignment operator
     */
    SubmessageHeader& operator=(SubmessageHeader&&) noexcept = default;

    /**
     * @brief destructor
     */
    ~SubmessageHeader()
    {};

    // contents
    std::uint8_t submessageId;
    std::uint32_t submessageLength;  // without header
    std::uint8_t flags;
    bool is_last;

    // misc information
    std::uint32_t length;

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
    SubmessageBase():
        subMsgHeader()
    {};

    SubmessageBase(
        std::uint8_t subMsgId, 
        std::uint32_t subMsgLength, 
        bool isLast
    ):
        subMsgHeader(subMsgId, subMsgLength, isLast)
    {};

    // contents
    SubmessageHeader subMsgHeader;
};

class DataFrag: public SubmessageBase
{
    public:
    /**
     * @brief constructor for empty submessage object
     */
    DataFrag():
        SubmessageBase(DATA_FRAG, this->length, false)
    {};

    /**
     * @brief Construct a new Data Frag object
     * 
     * @param readerID 
     * @param writerID 
     * @param writerSN 
     * @param fragmentNum 
     * @param dataSize 
     * @param fragmentSize 
     * @param payload 
     * @param sampleTimestamp 
     */
    DataFrag(
        std::uint32_t readerID, 
        std::uint32_t writerID,
        std::uint64_t writerSN, 
        std::uint32_t fragmentNum, 
        std::uint32_t dataSize, 
        std::uint16_t fragmentSize, 
        unsigned char *payload, 
        std::chrono::system_clock::time_point sampleTimestamp
    ):
        SubmessageBase(DATA_FRAG, this->length, false),
        readerID(readerID),
        writerID(writerID), 
        writerSequenceNumber(writerSN),
        fragmentNumber(fragmentNum),
        fragmentsInSubmessage(1),
        sampleSize(dataSize),
        fragmentSize(fragmentSize),
        timestamp(sampleTimestamp),
        serializedPayload(payload, payload + fragmentSize)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSequenceNumber) +
                       sizeof(fragmentNumber) +
                       sizeof(fragmentsInSubmessage) +
                       sizeof(timestamp) +
                       fragmentSize;
    };

    /**
     * @brief Construct a new Data Frag object
     * 
     * @param readerID 
     * @param writerID 
     * @param change 
     * @param sf 
     */
    DataFrag(
        std::uint32_t readerID, 
        std::uint32_t writerID, 
        CacheChange &change,
        SampleFragment &sf
    ):
        SubmessageBase(DATA_FRAG, this->length, false),
        readerID(readerID),
        writerID(writerID), 
        writerSequenceNumber(change.sequenceNumber),
        fragmentNumber(sf.number),
        fragmentsInSubmessage(1),
        sampleSize(change.sampleSize),
        fragmentSize(sf.size),
        timestamp(change.arrivalTime),
        serializedPayload(sf.payload.data(), sf.payload.data() + sf.size)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSequenceNumber) +
                       sizeof(fragmentNumber) +
                       sizeof(fragmentsInSubmessage) +
                       sizeof(timestamp) +
                       fragmentSize;
    };

    /**
     * @brief Construct a new Data Frag from a MessageNet_t object
     * 
     * @param msg 
     */
    DataFrag(
        MessageNet_t* msg
    ):
        SubmessageBase(DATA_FRAG, this->length, false)
    {
        this->netToData(msg);
    };

    /**
     * @brief disable copy constructor
     */
    DataFrag(DataFrag &data) = delete;

    /**
     * @brief disable copy assignment
     */
    DataFrag& operator=(const DataFrag &data) = delete;
    
    /**
     * @brief Allow moving DataFrag into variants/containers
     */
    DataFrag(DataFrag&&) noexcept = default;
    DataFrag& operator=(DataFrag&&) noexcept = default;

    std::uint32_t readerID;                 // Identifies the Reader entity that is being informed of the change to the data-object. 
    std::uint32_t writerID;                 // Identifies the Writer entity that made the change to the data- object.
    std::uint64_t writerSequenceNumber;     // Uniquely identifies the change and the relative order for all changes made by the Writer.
    std::uint32_t fragmentNumber;           // Indicates the starting fragment for the series of fragments in the serializedPayload field
    std::uint16_t fragmentsInSubmessage;    // The number of consecutive fragments contained in this Submessage
    std::uint32_t sampleSize;               // The total size in bytes of the original data before fragmentation.
    std::uint16_t fragmentSize;             // The size of an individual fragment in bytes. The maximum fragment size equals 64K.
    // parameterList inlineQos;
    std::vector<unsigned char> serializedPayload;  // actual payload
    std::chrono::system_clock::time_point timestamp; // timestamp signaling the arrival of the sample at the writer, required at the reader for determining deadline violations. Requires time sync between nodes hosting writer and reader 

    // misc information
    std::uint32_t length;

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
    
    struct fragmentNumberSet {

        fragmentNumberSet(): base(0)
        {
            memset(bitmap, 0, NACK_BITMAP_SIZE);
        };

        fragmentNumberSet(const fragmentNumberSet &other)
        {
            this->base = other.base;
            memcpy(this->bitmap, other.bitmap, NACK_BITMAP_SIZE);
        };

        // New: Explicitly default the Move Constructor and mark noexcept
        fragmentNumberSet(fragmentNumberSet&&) noexcept = default;
        fragmentNumberSet& operator=(fragmentNumberSet&&) noexcept = default;
        
        // New: Explicitly default the Copy Assignment (needed for Rule of Five/Three)
        fragmentNumberSet& operator=(const fragmentNumberSet &other) = default;

        std::uint32_t base;
        unsigned char bitmap[NACK_BITMAP_SIZE];
    };
    
    /**
     * @brief (default) Construct a new Nack Frag object
     * 
     */
    NackFrag():
        SubmessageBase(NACK_FRAG, this->length, false)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSequenceNumber) +
                       sizeof(fragmentStatus) +
                       sizeof(count);
    };

    /**
     * @brief Construct a new Nack Frag object
     * 
     * @param readerID 
     * @param writerID 
     * @param writerSN 
     * @param status 
     * @param NackFragCount 
     */
    NackFrag(
        std::uint32_t readerID, 
        std::uint32_t writerID,
        std::uint64_t writerSN,
        fragmentNumberSet &status,
        std::uint32_t NackFragCount
    ):
        SubmessageBase(NACK_FRAG, this->length, false),
        readerID(readerID),
        writerID(writerID),
        writerSequenceNumber(writerSN),
        fragmentStatus(status),
        count(NackFragCount)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSequenceNumber) +
                       sizeof(fragmentStatus) +
                       sizeof(count);
    };

    /**
     * @brief Construct a new Nack Frag from a MessageNet_t object
     * 
     * @param msg 
     */
    NackFrag(
        MessageNet_t* msg
    ):
        SubmessageBase(NACK_FRAG, this->length, false)
    {
        this->netToNack(msg);
    }; 

    /**
     * @brief disable copy constructor
     */
    NackFrag(NackFrag &nack) = delete;
    /**
     * @brief disable copy assignment
     */
    NackFrag& operator=(const NackFrag &nack) = delete;
    
    /**
     * @brief Allow moving NackFrag into variants/containers
     */
    NackFrag(NackFrag&&) noexcept = default;
    NackFrag& operator=(NackFrag&&) noexcept = default;

    /**
     * @brief default destructor
     */
    ~NackFrag()
    {};

    // contents
    std::uint32_t readerID;                     // Identifies the Reader entity that requests to receive certain fragments.
    std::uint32_t writerID;                     // Identifies the Writer entity that is the target of the NackFrag message. This is the Writer Entity that is being asked to re-send some fragments.
    std::uint64_t writerSequenceNumber;         // The sequence number for which some fragments are missing.
    fragmentNumberSet fragmentStatus;                   // Communicates the state of the reader to the writer. The fragment numbers that appear in the set indicate missing fragments on the reader side. The ones that do not appear in the set are undetermined (could have been received or not).
    std::uint32_t count;                        // Reader's NackFrag counter // repurposed for returning the HB's lastFragmentNum
    
    // misc information
    std::uint32_t length;

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
    HeartbeatFrag():
        SubmessageBase(HEARTBEAT_FRAG, this->length, false)
    {
        this->length = sizeof(readerID) +
                       sizeof(writerID) +
                       sizeof(writerSequenceNumber) +
                       sizeof(highestFragmentNum) + 
                       sizeof(count);
    };

    /**
     * @brief Construct a new Heartbeat Frag object
     * 
     * @param readerID 
     * @param writerID 
     * @param writerSN 
     * @param highestFragmentNum 
     * @param HBFragCount 
     */
    HeartbeatFrag(
        std::uint32_t readerID, 
        std::uint32_t writerID,
        std::uint64_t writerSN, 
        std::uint32_t highestFragmentNum, 
        std::uint32_t HBFragCount
    ):
        SubmessageBase(HEARTBEAT_FRAG, this->length, false),
        readerID(readerID),
        writerID(writerID),
        writerSequenceNumber(writerSN),
        highestFragmentNum(highestFragmentNum),
        count(HBFragCount)
    {
        this->length = sizeof(this->readerID) +
                       sizeof(this->writerID) +
                       sizeof(this->writerSequenceNumber) +
                       sizeof(this->highestFragmentNum) + 
                       sizeof(this->count);
    };

    /**
     * @brief Construct a new Heartbeat Frag from a MessageNet_t object
     * 
     * @param msg 
     */
    HeartbeatFrag(
        MessageNet_t* msg
    ):
        SubmessageBase(HEARTBEAT_FRAG, this->length, false)
    {
        this->length = sizeof(this->readerID) +
                sizeof(this->writerID) +
                sizeof(this->writerSequenceNumber) +
                sizeof(this->highestFragmentNum) + 
                sizeof(this->count);
        this->netToHB(msg);
    };

    /**
     * @brief disable copy constructor
     */
    HeartbeatFrag(HeartbeatFrag &hb) = delete;

    /**
     * @brief disable copy assignment
     */
    HeartbeatFrag& operator=(const HeartbeatFrag &data) = delete;
    
    /**
     * @brief Allow moving HeartbeatFrag into variants/containers
     */
    HeartbeatFrag(HeartbeatFrag&&) noexcept = default;
    HeartbeatFrag& operator=(HeartbeatFrag&&) noexcept = default;

    /**
     * @brief default destructor
     */
    ~HeartbeatFrag()
    {};

    // contents
    std::uint32_t readerID;             // Identifies the Reader Entity that is being informed of the availability of fragments. Can be set to ENTITYID_UNKNOWN to indicate all readers for the writer that sent the message.
    std::uint32_t writerID;             // Identifies the Writer Entity that sent the Submessage.
    std::uint64_t writerSequenceNumber; // Identifies the sequence number of the data change for which fragments are available.
    std::uint32_t highestFragmentNum;   // All fragments up to and including this last (highest) fragment are available on the Writer for the change identified by writerSN.
    std::uint32_t count;   

    // misc information
    std::uint32_t length;

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

} //end namespace W2RP

#endif //MESSAGES_H