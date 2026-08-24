#ifndef W2RP_ActiveWaitTimer_h
#define W2RP_ActiveWaitTimer_h

#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>


class ActiveWaitTimer
{
    public:

        typedef std::chrono::steady_clock::duration Duration;

        ActiveWaitTimer()
        :
            active(false),
            t_mutex(),
            worker{}
        {}

        // Start the timer with a specific duration and function to call
        void start(Duration duration, std::function<bool()> function, bool immediateExecution = true)
        {
            std::unique_lock<std::mutex> lock(t_mutex);
            if(!active)
            {
                stop_impl();

                active = true;

                worker = std::thread([this, duration, function, immediateExecution]() {
                    
                    auto start_time = std::chrono::steady_clock::now();
                    if(immediateExecution)
                    {
                        start_time = start_time - duration; // execute as soon as possible
                    }
              
                    while (active)
                    {
                        auto current_time = std::chrono::steady_clock::now();
                        if (current_time - start_time >= duration)
                        {
                            start_time = std::chrono::steady_clock::now();
                            bool restart_ = function();
                            if(!restart_)
                            {
                                break;
                            }
                        }
                    }
                    active = false;
                });
            }
            lock.unlock();
        }

        // Stop the timer
        void stop()
        {
            std::lock_guard<std::mutex> lock(t_mutex);
            stop_impl();
        }

        ~ActiveWaitTimer()
        {
            stop();
        }

    private:
        std::atomic<bool> active;
        std::thread worker;
        std::mutex t_mutex;

        void stop_impl()
        {
            active = false;
            if (worker.joinable())
            {
                worker.join();
            }
        }
};

#endif