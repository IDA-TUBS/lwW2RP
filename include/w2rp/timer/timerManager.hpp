#ifndef TimerManager_h
#define TimerManager_h

#include <w2rp/log.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>
#include <iostream>
#include <mutex>

namespace w2rp{

class TimerManager
{
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
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param duration timer interval
     * @param repeat periodic/single shot activation?
     * @param function event handler
     * @param args handler arguments
     * @return size_t timer id
     */
    template <typename FunctionType, typename... Args>
    size_t registerSteadyTimer(const Duration& duration, bool repeat, FunctionType&& function, Args&&... args)
    {
        size_t timer_id = assignID();
       
        std::unique_lock<std::mutex> lock(m_mutex);
        timer_repeat.insert({timer_id, repeat});
        lock.unlock();

        // Create a timer and bind it to the function
        auto timer = std::make_shared<boost::asio::steady_timer>(timer_io);
        configureSteadyTimer(timer, timer_id, duration, function, args...);        

        // Add the timer to the list of timers
        lock.lock();
        m_steady_timers.emplace(timer_id, std::move(timer));
        
        return timer_id;
    }

    /**
     * @brief register a timed event based on the system clock
     * 
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param time_point trigger time point
     * @param function event handler
     * @param args handler arguments
     * @return size_t timer id
     */
    template <typename FunctionType, typename... Args>
    size_t registerSystemTimer(const TimePoint& time_point, FunctionType&& function, Args&&... args)
    {
        size_t timer_id = assignID();

        // Create a timer and bind it to the function
        auto timer = std::make_shared<boost::asio::system_timer>(timer_io);
        configureSystemTimer(timer, timer_id, time_point, function, args...);

        // Add the timer to the list of timers
        std::unique_lock<std::mutex> lock(m_mutex);
        m_system_timers.emplace(timer_id, std::move(timer));
        
        return timer_id;
    }

    /**
     * @brief restart a registered steady timer 
     * 
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param duration timer interval
     * @param function event handler
     * @param args handler arguments
     * @return size_t timer id
     */
    template <typename FunctionType, typename... Args>
    void restartSteadyTimer(size_t timer_id, const Duration& duration, FunctionType&& function, Args&&... args)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto timer = m_steady_timers[timer_id];
        lock.unlock();

        configureSteadyTimer(timer, timer_id, duration, function, args...);
    };

    /**
     * @brief restart a registered system timer
     * 
     * @tparam FunctionType generic event handler
     * @tparam Args generic event handler arguments
     * @param time_point trigger time point
     * @param function event handler
     * @param args handler arguments
     * @return size_t timer id
     */
    template <typename FunctionType, typename... Args>
    void restartSystemTimer(size_t timer_id, const TimePoint& time_point, FunctionType&& function, Args&&... args)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto timer = m_system_timers[timer_id];
        lock.unlock();

        configureSystemTimer(timer, timer_id, time_point, function, args...);
    };

    /**
     * @brief unregister a steady clock based timed event 
     * 
     * @param id timer id
     */
    void unregisterSteadyTimer(size_t id);

    /**
     * @brief unregister a system clock based timed event
     * 
     * @param id timer id
     */
    void unregisterSystemTimer(size_t id);

    private:

    /**
     * @brief determine timer id for registration
     * 
     * @return size_t timer id
     */
    size_t assignID();

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
    template<typename FunctionType, typename... Args>
    void configureSteadyTimer(std::shared_ptr<boost::asio::steady_timer> timer, size_t timer_id, Duration duration, FunctionType&& function, Args&&... args)
    {
        timer->expires_after(duration);
        timer->async_wait(
            [timer, timer_id, function, duration, this, args...](const boost::system::error_code& ec) {
                if (!ec) {
                    auto task = std::bind(function, args...);
                    task();                
                    auto entry = timer_repeat.find(timer_id);
                    if(entry->second == true)
                    {
                        logInfo("Re-register intervall timer: " << timer_id)
                        configureSteadyTimer(timer, timer_id, duration, function, args...);
                    }
                }
                else
                {
                    logInfo("Cancel called: " << timer_id)
                }
        });
    };

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
    template<typename FunctionType, typename... Args>
    void configureSystemTimer(std::shared_ptr<boost::asio::system_timer> timer, size_t timer_id, TimePoint time_point, FunctionType&& function, Args&&... args)
    {
        timer->expires_at(time_point);
        timer->async_wait(
            [timer, timer_id, function, this, args...](const boost::system::error_code& ec) {
            if (!ec) {
                auto task = std::bind(function, args...);
                task();                
            }
            else
            {
                logInfo("Cancel called: " << timer_id)
            }
        });
    };

    boost::asio::io_service timer_io;
    boost::asio::io_service::work io_work;
    
    std::thread executor;
    
    std::map<size_t, bool> timer_repeat;
    std::map<size_t, std::shared_ptr<boost::asio::steady_timer>> m_steady_timers;
    
    std::map<size_t, std::shared_ptr<boost::asio::system_timer>> m_system_timers;

    std::mutex m_mutex;
    
    size_t timer_count_ = 0;
    size_t event_id_count = 0;
};
}; // End w2rp namespace

#endif