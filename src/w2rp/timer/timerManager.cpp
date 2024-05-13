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

void TimerManager::unregisterSteadyTimer(size_t id)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto timer_ptr = m_steady_timers[id];

    logInfo("Removing timer: " << id)
    timer_ptr->cancel();
    
    m_steady_timers.erase(id);

    lock.unlock();
};

void TimerManager::unregisterSystemTimer(size_t id)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto timer_ptr = m_system_timers[id];

    logInfo("Removing timer: " << id)
    timer_ptr->cancel();
   
    m_system_timers.erase(id);

    lock.unlock();
};

/* ----------------- PrivateMethods ------------------------- */
size_t TimerManager::assignID()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    size_t id = event_id_count;
    event_id_count++;
    return id;
}
