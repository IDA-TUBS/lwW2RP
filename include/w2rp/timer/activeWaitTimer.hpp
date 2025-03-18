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
        void start(Duration duration, std::function<bool()> function)
        {
            std::unique_lock<std::mutex> lock(t_mutex);
            if(!active)
            {
                stop();  // Ensure any previous timer is stopped

                active = true;
                worker = std::thread([this, duration, function]() {
                    auto start_time = std::chrono::steady_clock::now();
                    while (active)
                    {
                        auto current_time = std::chrono::steady_clock::now();
                        if (current_time - start_time >= duration)
                        {
                            start_time = std::chrono::steady_clock::now();
                            bool restart_ = function();
                            if(!restart_)
                            {
                                // active = false;
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
            active = false;
            if (worker.joinable())
            {
                worker.join();
            }
        }

        ~ActiveWaitTimer()
        {
            stop();
        }

    private:
        std::atomic<bool> active;
        std::thread worker;
        std::mutex t_mutex;
};

#endif