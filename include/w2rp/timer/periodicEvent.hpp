#ifndef PeriodicEvent_h
#define PeriodicEvent_h

#include <w2rp/timer/timerManager.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>
#include <iostream>
#include <mutex>

namespace w2rp{

template<typename... Args>
class PeriodicEvent
{
    /************************************************************************/
    public:


    /**
     * @brief empty constructor
     */
    PeriodicEvent() 
    {};

    /**
     * @brief Construct a new periodic Timed Event object
     * 
     * @param interval event cycle
     * @param callback handler routine
     * @param service management entity
     */
    PeriodicEvent(
        TimerManager& service,
        std::chrono::microseconds interval,
        std::function<bool()> callback,
        Args... args
    ):
        service_(service),
        interval_(interval),
        callback_(std::bind(callback, args...)),    
        args_(std::make_tuple(args...))
    {
        auto timerCallback = [&]() { return eventCallback(); }; /*std::bind(&TimedEvent<Args...>::eventCallback, this);*/
        id = service_.registerTimer(interval, timerCallback);
        isActive_ = true;
    };
    
    /**
     * @brief Destroy the Timed Event object
     * 
     */
    ~PeriodicEvent()
    {
        service_.unregisterSteadyTimer(id);
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

    bool isActive()
    {
        return isActive_;
    }

    bool eventCallback()
    {
        isActive_ = callback_();
        return isActive_;
    }

    /************************************************************************/
    private:
    
    size_t id;
    TimerManager& service_;
    std::chrono::microseconds interval_;
    std::function<bool()> callback_;
    std::tuple<Args...> args_;
    bool isActive_;
};

// Deduction guide for TimedEvent Class
template<typename... Args>
PeriodicEvent(TimerManager&, std::chrono::microseconds, std::function<bool()>, Args...) -> PeriodicEvent<Args...>;

}; // End w2rp namespace

#endif