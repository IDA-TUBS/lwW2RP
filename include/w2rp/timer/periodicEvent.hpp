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
     * @param autoStart (true): automatic timer start (false): manual timer start ->restart()
     */
    PeriodicEvent(
        TimerManager& service,
        std::chrono::microseconds interval,
        std::function<bool()> callback,
        TimerType timer,
        bool autoStart,
        Args... args
    ):
        service_(service),
        interval_(interval),
        callback_(std::bind(callback, args...)),    
        args_(std::make_tuple(args...))
    {
        auto timerCallback = [&]() { return eventCallback(); }; 
        id = service_.registerTimer(interval, timerCallback, timer, autoStart);
        if(autoStart)
        {
            isActive_ = true;  
        }
        else
        {
            isActive_ = false;
        }
    };

    PeriodicEvent(
        TimerManager& service,
        std::chrono::microseconds interval,
        std::function<bool()> callback,
        Args... args
    ):
        PeriodicEvent(service, interval, callback, STEADY_TIMER, true, args...)
    {};
    
    /**
     * @brief Destroy the Timed Event object
     * 
     */
    ~PeriodicEvent()
    {
        service_.unregisterTimer(id);
    };

    /**
     * @brief restart the timed event
     * 
     */
    void restart()
    {
        auto timerCallback = [&]() { return eventCallback(); }; 
        service_.restartTimer(id, interval_, timerCallback);
        isActive_ = true;
    };

    /**
     * @brief restart the timed event
     * 
     * @param interval interval for event activation
     */
    void restart(std::chrono::microseconds interval)
    {
        interval_ = interval;
        restart();
    }

    /**
     * @brief cancel the timed event
     * 
     */
    void cancel()
    {
        service_.cancelTimer(id);
        isActive_ = false;
    };

    /**
     * @brief Check whether the event is active
     * 
     * @return true active, timer running
     * @return false inactive, timer holding
     */
    bool isActive()
    {
        return isActive_;
    }

    /**
     * @brief Event handler callback. Executed when the event is triggered.
     * 
     * @return true restart timer  
     * @return false do not restart timer
     */
    bool eventCallback()
    {
        isActive_ = callback_();
        return isActive_;
    }

    /**
     * @brief Set the Interval object
     * 
     * @param interval activation interval in [us]
     */
    void setInterval(std::chrono::microseconds interval)
    {
        interval_ = interval;
    };

    /**
     * @brief Get the Interval object
     * 
     * @return std::chrono::microseconds activation interval [us]
     */
    std::chrono::microseconds getInterval()
    {
        return interval_;
    }

    /************************************************************************/
    private:
    
    timerID id;
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