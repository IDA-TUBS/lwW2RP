#ifndef W2RP_TimerManager_h
#define W2RP_TimerManager_h

#include <w2rp/log.hpp>
#include <w2rp/timer/activeWaitTimer.hpp>
#include <w2rp/timer/nanosleepTimer.hpp>

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>
#include <iostream>
#include <mutex>

namespace w2rp{

enum TimerType
{
    STEADY_TIMER = 0,
    SYSTEM_TIMER,
    ACTIVEWAIT_TIMER,
    NANOSLEEP_TIMER
};

typedef std::pair<TimerType,size_t> timerID;

class TimerManager
{
    /************************************************************************/
    public:
    
    typedef std::chrono::system_clock::time_point TimePoint;
    typedef std::chrono::steady_clock::duration Duration;

    /**
     * @brief Construct a new Timer Manager object
     * 
     */
    TimerManager();
    
    /**
     * @brief Destroy the Timer Manager object
     * 
     */
    ~TimerManager();

    /**
     * @brief start timed event processing
     * 
     */
    void start();

    /**
     * @brief stop timed event processing
     * 
     */
    void stop();
    
    /**
     * @brief register a timed event based on steady clock
     * 
     * @param duration timer interval
     * @param function event handler
     * @param timer TimerType {STEADY, ACTIVEWAIT}
     * @param autoStart [bool] (true): timer allocation + start (false) timer allocation only
     * @return timerID timer id object
     */
    timerID registerTimer(
        const Duration& duration, 
        std::function<bool()> function,
        TimerType timer,
        bool autoStart = true,
        bool immediateExecution = true
    );

    /**
     * @brief register a timed event based on the system clock
     * 
     * @param time_point trigger time point
     * @param function event handler
     * @return timerID timer id object
     */
    timerID registerTimer(
        const TimePoint& time_point, 
        std::function<void()> function
    );

    /**
     * @brief unregister a timer
     * 
     * @param timer_id timerID object belonging to the timer to be removed
     */
    void unregisterTimer(timerID timer_id);    

    /**
     * @brief cancel a running timer
     * 
     * @param timer_id timer ID object of the timer to be cancelled
     */
    void cancelTimer(timerID timer_id);

    /**
     * @brief restart a registered interval timer 
     *      
     * @param timerID timer id object
     * @param duration timer interval
     * @param function event handler
     */
    void restartTimer(
        timerID timer_id, 
        const Duration& duration, 
        std::function<bool()> function,
        bool immediateExecution
    );

    /**
     * @brief restart a registered system timer
     * 
     * @param timerID timer id object
     * @param time_point trigger time point
     * @param function event handler
     */
    void restartTimer(
        timerID timer_id, 
        const TimePoint& time_point, 
        std::function<void()> function
    );

    /************************************************************************/
    private:

    /*----------------------------- Methods --------------------------*/

    /**
     * @brief determine timer id for registration
     * 
     * @param t_type type of the timer 
     * 
     * @return size_t timer id
     */
    timerID assignID(TimerType t_type);

    /**
     * @brief configure steady timer for a registered timed event
     * 
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param timer pointer to timer object
     * @param timer_id timer id 
     * @param duration timer interval
     * @param function event handler
     * @param args handler arguments
     */
    void configureTimer(
        std::shared_ptr<boost::asio::steady_timer> timer, 
        size_t timer_id, 
        Duration duration, 
        std::function<bool()> function,
        bool immediateExecution = true
    );

    /**
     * @brief configure system timer for registered timed event
     * 
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param timer pointer to timer object
     * @param timer_id timer id 
     * @param time_point activation time point
     * @param function event handler
     * @param args handler arguments
     */
    void configureTimer(
        std::shared_ptr<boost::asio::system_timer> timer, 
        size_t timer_id, 
        TimePoint time_point, 
        std::function<void()> function
    );

    /**
     * @brief Get the Steady Timer object
     * 
     * @param id 
     * @return std::shared_ptr<boost::asio::steady_timer> 
     */
    std::shared_ptr<boost::asio::steady_timer> getSteadyTimer(size_t id);

    /**
     * @brief Get the System Timer object
     * 
     * @param id 
     * @return std::shared_ptr<boost::asio::system_timer> 
     */
    std::shared_ptr<boost::asio::system_timer> getSystemTimer(size_t id);

    /**
     * @brief Get the ActiveWaitTimer object 
     * 
     * @param id 
     * @return std::shared_ptr<ActiveWaitTimer> 
     */
    std::shared_ptr<ActiveWaitTimer> getActiveWaitTimer(size_t id);

    /**
     * @brief Get the NanosleepTimer object 
     * 
     * @param id 
     * @return std::shared_ptr<NanosleepTimer> 
     */
    std::shared_ptr<NanosleepTimer> getNanosleepTimer(size_t id);

    /**
     * @brief register a timed event based on a steady timer
     * 
     * @param duration timer interval
     * @param function event handler
     * @param timer TimerType {STEADY, ACTIVEWAIT}
     * @param autoStart [bool] (true): timer allocation + start (false) timer allocation only
     * @return timerID timer id object
     */
    timerID registerSteadyTimer(
        const Duration& duration, 
        std::function<bool()> function,
        bool autoStart = true,
        bool immediateExecution = true
    );

    /**
     * @brief register a timed event based on a active wait timer
     * 
     * @param duration timer interval
     * @param function event handler
     * @param autoStart [bool] (true): timer allocation + start (false) timer allocation only
     * @param immediateExecution [bool] (true): immediate execution of the event handler upon timer start (false): first execution after duration has passed
     * @return timerID 
     */
    timerID registerActiveWaitTimer(
        const Duration& duration, 
        std::function<bool()> function,
        bool autoStart = true,
        bool immediateExecution = true
    );

    /**
     * @brief register a timed event based on a nanosleep timer
     * 
     * @param duration timer interval
     * @param function event handler
     * @param autoStart [bool] (true): timer allocation + start (false) timer allocation only
     * @param immediateExecution [bool] (true): immediate execution of the event handler upon timer start (false): first execution after duration has passed
     * @return timerID 
     */
    timerID registerNanosleepTimer(
        const Duration& duration, 
        std::function<bool()> function,
        bool autoStart = true,
        bool immediateExecution = true
    );

    /**
     * @brief restart a registered steady timer 
     *      
     * @param timerID timer id object
     * @param duration timer interval
     * @param function event handler
     * @param immediateExecution [bool] (true): immediate execution of the event handler upon timer start (false): first execution after duration has passed
     */
    void restartSteadyTimer(
        timerID timer_id, 
        const Duration& duration, 
        std::function<bool()> function,
        bool immediateExecution = true
    );

    /**
     * @brief restart a registered active wait timer 
     *      
     * @param timerID timer id object
     * @param duration timer interval
     * @param function event handler
     * @param immediateExecution [bool] (true): immediate execution of the event handler upon timer start (false): first execution after duration has passed
     */
    void restartActiveWaitTimer(
        timerID timer_id, 
        const Duration& duration, 
        std::function<bool()> function,
        bool immediateExecution = true
    );

    /**
     * @brief restart a registered nanosleep timer 
     *      
     * @param timerID timer id object
     * @param duration timer interval
     * @param function event handler
     * @param immediateExecution [bool] (true): immediate execution of the event handler upon timer start (false): first execution after duration has passed
     */
    void restartNanosleepTimer(
        timerID timer_id, 
        const Duration& duration, 
        std::function<bool()> function,
        bool immediateExecution = true
    );

    /**
     * @brief remove a timer from
     * 
     * @param timer_id 
     */
    void removeTimer(timerID timer_id);


    /*----------------------------- Attributes --------------------------*/
    boost::asio::io_service timer_io;
    boost::asio::io_service::work io_work;
    
    std::thread executor;
    
    std::map<size_t, std::shared_ptr<boost::asio::steady_timer>> m_steady_timers;
    std::map<size_t, std::shared_ptr<boost::asio::system_timer>> m_system_timers;
    std::map<size_t,  std::shared_ptr<ActiveWaitTimer>> m_activewait_timers;
    std::map<size_t, std::shared_ptr<NanosleepTimer>> m_nanosleep_timers;

    std::mutex m_mutex;
    
    size_t timer_count_ = 0;
    size_t event_id_count = 0;
};
}; // End w2rp namespace

#endif