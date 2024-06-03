#ifndef W2RP_QOS_h
#define W2RP_QOS_h

namespace w2rp{

typedef enum
{
    NONE = 0,
    FIXED,
    ADAPTIVE_LOW_PDR,
    ADAPTIVE_HIGH_PDR
} PrioritizationMode;

} // end namespace w2rp

#endif