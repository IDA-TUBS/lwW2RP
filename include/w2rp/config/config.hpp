#ifndef W2RP_CONFIG_h
#define W2RP_CONFIG_h

namespace w2rp{
namespace config{


/********************** General ***************************/

#define DEADLINE "DEADLINE"
#define SIZE_CACHE "SIZE_CACHE"
#define UUID "UUID"
#define ADDRESS "ADDRESS"
#define PORT "PORT"

/*********************** Reader ***************************/
#define READER "READER"
#define RESPONSE_DELAY "RESPONSE_DELAY"
#define PRIORITY "PRIORITY"
#define WRITER "WRITER"

/*********************** Writer ***************************/
#define WRITER "WRITER"
#define FRAGMENT_SIZE "FRAGMENT_SIZE"
#define SHAPING_TIME "SHAPING_TIME"
#define NACK_SUPPRESSION_DURATION "NACK_SUPPRESSION_DURATION"
#define NUMBER_READERS "NUMBER_READERS"
#define PRIO_MODE "PRIO_MODE"
#define READERS "READERS"

}; // end namespace conifg
}; // end namespace w2rp

#endif