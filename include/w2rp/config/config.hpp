#ifndef W2RP_CONFIG_h
#define W2RP_CONFIG_h

namespace w2rp{
namespace config{


/********************** General ***************************/
#define DEADLINE "DEADLINE [us]"
#define SIZE_CACHE "SIZE_CACHE"
#define HOST "HOST"
#define ADDRESS "ADDRESS"
#define PORT "PORT"
#define READER "READER"
#define WRITER "WRITER"
#define READERS "READERS"

/*********************** Reader ***************************/
#define RESPONSE_DELAY "RESPONSE_DELAY [us]"
#define CHECK_LIVELINESS "CHECK_LIVELINESS"

/*********************** Writer ***************************/
#define FRAGMENT_SIZE "FRAGMENT_SIZE [Byte]"
#define SHAPING_TIME "SHAPING_TIME [us]"
#define NACK_SUPPRESSION_DURATION "NACK_SUPPRESSION_DURATION [us]"
#define TIMEOUT_DURATION "TIMEOUT_DURATION [us]"
#define NUMBER_READERS "NUMBER_READERS"
#define AGGREGATION_SIZE "AGGREGATION_SIZE"

/************** Misc. parameters **************************/
#define NACK_BITMAP_SIZE 128 // in Bytes. Allows handling of NACK_BITMAP_SIZE*8 fragments

}; // end namespace conifg
}; // end namespace w2rp

#endif