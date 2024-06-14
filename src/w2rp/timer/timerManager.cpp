#include <w2rp/timer/timerManager.hpp>

using namespace w2rp;

/* --------------------------- Public Methods ----------------------------------- */
TimerManager::TimerManager()
:
    timer_io(),
    io_work(timer_io),
    m_steady_timers(),
    m_system_timers(),
    m_mutex(),
    executor{}
{

}

TimerManager::~TimerManager()
{
    stop();
}

void TimerManager::start()
{
    logInfo("TimerManager active")
    executor = std::thread([this](){timer_io.run();});
}

void TimerManager::stop()
{
    // Stop the worker threads
    timer_io.stop();
    executor.join();
}

timerID TimerManager::registerTimer(const Duration& duration, std::function<bool()> function, bool autoStart)
{
    timerID timer_id = assignID(STEADY_TIMER);
    
    // Create a timer and bind it to the function
    auto timer = std::make_shared<boost::asio::steady_timer>(timer_io);
    
    if(autoStart)
    {
        configureTimer(timer, timer_id.second, duration, function);      
    }

    // Add the timer to the list of timers
    std::unique_lock<std::mutex> lock(m_mutex);
    m_steady_timers.emplace(timer_id.second, std::move(timer));
    
    return timer_id;
}

timerID TimerManager::registerTimer(const TimePoint& time_point, std::function<void()> function)
{
    timerID timer_id = assignID(SYSTEM_TIMER);

    // Create a timer and bind it to the function
    auto timer = std::make_shared<boost::asio::system_timer>(timer_io);
    if(time_point > std::chrono::system_clock::time_point::min())
    {
        logDebug("[TimerManager] (registerTime) configureTimer called")
        configureTimer(timer, timer_id.second, time_point, function);
    }

    // Add the timer to the list of timers
    std::unique_lock<std::mutex> lock(m_mutex);
    m_system_timers.emplace(timer_id.second, std::move(timer));
    
    return timer_id;
}

void TimerManager::unregisterTimer(timerID timer_id)
{
    logInfo("Removing timer: " << timer_id.second << ": " << timer_id.first)

    std::unique_lock<std::mutex> lock(m_mutex);

    cancelTimer(timer_id);
    removeTimer(timer_id);

    timer_count_--;
}

void TimerManager::cancelTimer(timerID timer_id)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    switch (timer_id.first)
    {
        case STEADY_TIMER:
            {
                auto timer_entry = getSteadyTimer(timer_id.second);
                timer_entry->cancel();
                timer_entry->expires_at(std::chrono::steady_clock::now());
            }
            break;

        case SYSTEM_TIMER:
            {
                auto timer_entry = getSystemTimer(timer_id.second);
                timer_entry->cancel();
                timer_entry->expires_at(std::chrono::system_clock::now());
            }
            break;

        default:
            // nothing to do
            break;
    }
    
    lock.unlock();
}

void TimerManager::restartTimer(timerID timer_id, const Duration& duration, std::function<bool()> function)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto timer = getSteadyTimer(timer_id.second);
    lock.unlock();

    if(timer)
    {
        configureTimer(timer, timer_id.second, duration, function);
    }
    else
    {
        logInfo("restartTimer: Timer object not available")
    }

}

void TimerManager::restartTimer(timerID timer_id, const TimePoint& time_point, std::function<void()> function)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto timer = getSystemTimer(timer_id.second);
    lock.unlock();

    if(timer)
    {
        configureTimer(timer, timer_id.second, time_point, function);
    }
    else
    {
        logInfo("restartTimer: Timer object not available")
    }
}

/* ----------------- PrivateMethods ------------------------- */
timerID TimerManager::assignID(TimerType t_type)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    size_t id = event_id_count;
    event_id_count++;
    timer_count_++;
    lock.unlock();
    
    timerID timer_id(t_type, id); 

    return timer_id;
}

void TimerManager::configureTimer(std::shared_ptr<boost::asio::steady_timer> timer, size_t timer_id, Duration duration, std::function<bool()> function)
{
    timer->expires_after(duration);
    timer->async_wait(
        [timer, timer_id, function, duration, this](const boost::system::error_code& ec) {
            if (!ec) {
                auto task = std::bind(function);
                bool restart_ = task();                
                if(restart_ == true)
                {
                    // logInfo("Re-register intervall timer: " << timer_id)
                    configureTimer(timer, timer_id, duration, function);
                }
            }
            else
            {
                logInfo("Cancel called: " << timer_id)
            }
    });
}

void TimerManager::configureTimer(std::shared_ptr<boost::asio::system_timer> timer, size_t timer_id, TimePoint time_point, std::function<void()> function)
{
    timer->expires_at(time_point);
    timer->async_wait(
        [timer, timer_id, function, this](const boost::system::error_code& ec) {
        if (!ec) {
            auto task = std::bind(function);
            task();                
        }
        else
        {
            logInfo("Cancel called: " << timer_id)
        }
    });
}

std::shared_ptr<boost::asio::steady_timer> TimerManager::getSteadyTimer(size_t id)
{
    auto timer_entry = m_steady_timers.find(id);
    if(timer_entry != m_steady_timers.end())
    {
        return timer_entry->second;
    }
    else
    {
        return nullptr;
    }
}

std::shared_ptr<boost::asio::system_timer> TimerManager::getSystemTimer(size_t id)
{
    auto timer_entry = m_system_timers.find(id);
    if(timer_entry != m_system_timers.end())
    {
        return timer_entry->second;
    }
    else
    {
        return nullptr;
    }
}

void TimerManager::removeTimer(timerID timer_id)
{
    switch(timer_id.first)
    {
        case STEADY_TIMER:
            m_steady_timers.erase(timer_id.second);
            break;

        case SYSTEM_TIMER:
            m_system_timers.erase(timer_id.second);
            break;
    
        default:
            // nothing to do
            break;
    }
}