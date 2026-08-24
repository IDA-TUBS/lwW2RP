#ifndef W2RP_CHANGEFORREADER_H_
#define W2RP_CHANGEFORREADER_H_

#include <math.h>
#include <vector>
#include <chrono>
#include <memory>
#include <cstdint>
#include <deque>
#include <unordered_set>
#include <queue>
#include <functional>
#include <set>
#include <mutex>

#include <w2rp/cacheChange.hpp>
#include <w2rp/fragment/sampleFragmentState.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

class ChangeForReader
{
private:
    // reader entity ID
    std::uint32_t readerID;

    // mutex for object access
    std::mutex cache_mutex;

    // flag for signaling that all fragments have been acknowledged
    bool complete;

    // flag indicating whether all fragments have been transmitted at least once
    bool transmittedAll;

public:

    // storing fragment number that has been sent last
    std::uint32_t lastSent;

    // highest yet sent fragment number
    std::uint32_t highestSent;

    // lowest yet unsent/ fragment number
    std::uint32_t lowestNacked;

    // reference to corresponding CacheChange
    std::uint32_t sequenceNumber;

    // Maximum number of fragments that can be retransmitted for this change. 
    std::uint32_t retransmissionBudget;

    // Array for storing the unsent/nacked sample states
    std::vector<SampleFragmentState> fragmentStates;

    // quick access queue for sample fragments to be retransmitted
    std::priority_queue<
        std::uint32_t, 
        std::vector<std::uint32_t>, 
        std::greater<std::uint32_t>
    > unsentQueue;

    // quick access set for sample fragments to be sent
    std::deque<std::uint32_t> initialQueue;

    // quick access set for sample fragments to be acknowledged
    std::set<std::uint32_t> sentSet;

    /**
     * @brief empty default constructor
     */
    ChangeForReader();

    /**
     * @brief overloaded constructor
     *
     * @param change reference to the associated cache change
     * @param id of reader
     */
    ChangeForReader(
        CacheChange &change, 
        std::uint32_t id,
        std::uint32_t fragment_budget
    );

    /**
     * @brief destructor
     */
    ~ChangeForReader();

    /**
     * @brief reset all sent but unacknowledged fragments to state 'UNSENT', used by WiMEP
     */
    void resetSentFragments();

    /**
     * @brief check whether sample has been received or acknowledged in its entirety
     *
     * @return true if complete, else returns false
     */
    bool checkForCompleteness();

    /**
     * @brief get fragment number of next fragment that shall be transmitted
     *
     * @return fragment number of next fragment to be transmitted
     */
    std::uint32_t getFragmentForTx();

    /**
     * @brief set fragment to acknowledged
     * 
     * @param fragmentNumber fn of fragment to be updated
     * @return true if successful, else false
     */
    bool setAcked(std::uint32_t fragmentNumber);

    /**
     * @brief set fragments to acknowledged
     * 
     * @param lowestFragmentNumber fn of lowest fragment to be updated
     * @param highestFragmentNumber fn of highest fragment to be updated
     * @return true if successful, else false
     */
    bool setAcked(std::uint32_t lowestFragmentNumber, std::uint32_t highestFragmentNumber);

    /**
     * @brief set fragment to nacked
     * 
     * @param fragmentNumber fn of fragment to be updated
     * @param nackTimestamp timestamp when nack was received
     * @param nackSuppressionInterval duration for nack suppression
     * @return 0 if successful, -1 if fragment number is invalid, 1 if retransmission budget is exceeded
     */
    int setNacked(
        std::uint32_t fragmentNumber,
        std::chrono::system_clock::time_point nackTimestamp,
        std::chrono::system_clock::duration nackSuppressionInterval
    );

    /**
     * @brief return the number of unssent fragments
     * 
     * @return std::size_t 
     */
    std::size_t unsentCount();

    /**
     * @brief return the number of fragments that can still be retransmitted within the deadline
     * 
     * @return std::size_t 
     */
    std::size_t retransmissionBudgetCount();

}; 

}// end namespace

#endif // W2RP_CHANGEFORREADER_H_
