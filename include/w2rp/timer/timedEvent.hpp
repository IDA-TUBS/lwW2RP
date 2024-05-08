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
    public:
    
    /*
     * @brief empty constructor
     */
    TimedEvent() 
    {};


    /**
     * @brief Construct a new periodic Timed Event object
     * 
     * @param interval event cycle
     * @param callback handler routine
     * @param service management entity
     */
    TimedEvent(
        TimerManager& service,
        std::chrono::microseconds interval,
        std::function<void()> callback,
        Args... args
    ):
        service_(service),
        interval_(interval),
        callback_(callback),    
        args_(std::make_tuple(args...)),
        time_point_{}
    {
        id = service_.registerSteadyTimer(interval, true, callback, args...);
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
        callback_(callback),
        args_(std::make_tuple(args...)),
        interval_(0)
    {
        id = service_.registerSystemTimer(time_point, callback, args...);
    };

    
    /**
     * @brief Destroy the Timed Event object
     * 
     */
    ~TimedEvent()
    {
        if(interval_ == std::chrono::microseconds(0))
        {
            service_.unregisterSystemTimer(id);
        }
        else
        {
            service_.unregisterSteadyTimer(id);
        }
    };

    /**
     * @brief restart the timed event
     * 
     */
    void restart_timer()
    {
        //TBD
    };

    /**
     * @brief cancel the timed event
     * 
     */
    void cancel_timer()
    {
        // TBD
    };

    private:
    size_t id;
    TimerManager& service_;
    std::chrono::microseconds interval_;
    std::chrono::system_clock::time_point time_point_;
    std::function<void()> callback_;
    std::tuple<Args...> args_;
};

// Deduction guide for TimedEvent Class
template<typename... Args>
TimedEvent(TimerManager&, std::chrono::microseconds, std::function<void()>, Args...) -> TimedEvent<Args...>;

}; // End w2rp namespace

#endif