#ifndef W2RP_CONFIG_h
#define W2RP_CONFIG_h

namespace w2rp{
namespace config{


/********************** General ***************************/
#define DEADLINE "DEADLINE"
#define SIZE_CACHE "SIZE_CACHE"
#define HOST "HOST"
#define ADDRESS "ADDRESS"
#define PORT "PORT"
#define READER "READER"
#define WRITER "WRITER"
#define READERS "READERS"

/*********************** Reader ***************************/
#define RESPONSE_DELAY "RESPONSE_DELAY"
#define PRIORITY "PRIORITY"

/*********************** Writer ***************************/
#define FRAGMENT_SIZE "FRAGMENT_SIZE"
#define SHAPING_TIME "SHAPING_TIME"
#define NACK_SUPPRESSION_DURATION "NACK_SUPPRESSION_DURATION"
#define TIMEOUT "TIMEOUT"
#define NUMBER_READERS "NUMBER_READERS"
#define PRIO_MODE "PRIO_MODE"

}; // end namespace conifg
}; // end namespace w2rp

#endif