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
#define PRIORITY "PRIORITY"

/*********************** Writer ***************************/
#define FRAGMENT_SIZE "FRAGMENT_SIZE [Byte]"
#define SHAPING_TIME "SHAPING_TIME [us]"
#define NACK_SUPPRESSION_DURATION "NACK_SUPPRESSION_DURATION [us]"
#define TIMEOUT "TIMEOUT [us]"
#define NUMBER_READERS "NUMBER_READERS"
#define PRIO_MODE "PRIO_MODE"

/************** Misc. parameters **************************/
#define NACK_BITMAP_SIZE 64

}; // end namespace conifg
}; // end namespace w2rp

#endif