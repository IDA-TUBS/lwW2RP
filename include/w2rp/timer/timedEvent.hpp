#ifndef TimedEvent_h
#define TimedEvent_h

#include <w2rp/timer/timerManager.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>
#include <iostream>
#include <mutex>

namespace w2rp{

template<typename... Args>
class TimedEvent
{
    /************************************************************************/
    public:


    /**
     * @brief empty constructor
     */
    TimedEvent() 
    {};

    /**
     * @brief Construct a new single shot Timed Event object
     * 
     * @param callback handler routine
     * @param service management entity
     */
    TimedEvent(
        TimerManager& service,
        std::function<void()> callback,
        Args... args
    ):
        service_(service),
        time_point_(std::chrono::system_clock::time_point::min()),
        callback_(std::bind(callback, args...)),
        args_(std::make_tuple(args...))
    {
        auto timerCallback = std::bind(&TimedEvent<Args...>::eventCallback, this);
        id = service_.registerTimer(time_point_, timerCallback);
        isActive_ = false;
    };
    
    /**
     * @brief Construct a new single shot Timed Event object
     * 
     * @param time_point trigger time point
     * @param callback handler routine
     * @param service management entity
     */
    TimedEvent(
        TimerManager& service,
        std::chrono::system_clock::time_point time_point,
        std::function<void()> callback,
        Args... args
    ):
        service_(service),
        time_point_(time_point),
        callback_(std::bind(callback, args...)),
        args_(std::make_tuple(args...))
    {
        auto timerCallback = std::bind(&TimedEvent<Args...>::eventCallback, this);
        id = service_.registerTimer(time_point, timerCallback);
        isActive_ = true;
    };
    
    /**
     * @brief Destroy the Timed Event object
     * 
     */
    ~TimedEvent()
    {
        service_.unregisterTimer(id);
    };

    /**
     * @brief restart the timed event
     * 
     * @param time_point next activation time point
     */
    void restart(std::chrono::system_clock::time_point time_point)
    {
        auto timerCallback = std::bind(&TimedEvent<Args...>::eventCallback, this);
        time_point_ = time_point;
        service_.restartTimer(id, time_point_, timerCallback);
        isActive_ = true;
    };

    /**
     * @brief cancel the timed event
     * 
     */
    void cancel()
    {
        service_.cancelTimer(id);
        isActive_ = false;
    };

    bool isActive()
    {
        return isActive_;
    }

    void eventCallback()
    {
        callback_();
        isActive_ = false;
    }
    
    /************************************************************************/
    private:
    
    timerID id;
    TimerManager& service_;
    std::chrono::system_clock::time_point time_point_;
    std::function<void()> callback_;
    std::tuple<Args...> args_;
    bool isActive_;
};

// Deduction guide for TimedEvent Class
template<typename eventType, typename... Args>
TimedEvent(TimerManager&, std::chrono::microseconds, std::function<void()>, Args...) -> TimedEvent<Args...>;

}; // End w2rp namespace

#endif