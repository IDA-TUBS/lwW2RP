#ifndef W2RP_NanosleepTimer_h
#define W2RP_NanosleepTimer_h

#include <time.h>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

class NanosleepTimer
{
public:

    typedef std::chrono::steady_clock::duration Duration;

    NanosleepTimer()
    :
        active(false),
        worker{}
    {}


    void start(Duration period, std::function<bool()> callback, bool immediateExecution = true)
    {
        if (active.exchange(true)) {
            return;
        }

        worker = std::thread([this, period, callback, immediateExecution]() {
            timespec next {};
            clock_gettime(CLOCK_MONOTONIC, &next);

            if (immediateExecution) {
                if (!callback()) {
                    active = false;
                    return;
                }
            }

            while (active.load()) {
                add_duration(next, period);

                while (active.load()) {
                    int rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
                    if (rc == 0) {
                        break;
                    }
                    if (rc != EINTR) {
                        active = false;
                        return;
                    }
                }   

                std::unique_lock<std::mutex> lock(callback_mutex);
                if (!active.load()) {
                    break;
                }

                if (!callback()) {
                    active = false;
                    break;
                }
                lock.unlock();
            }
        });
    }

    void stop()
    {
        std::unique_lock<std::mutex> lock(callback_mutex);
        active = false;
        lock.unlock();
        if (worker.joinable()) {
            worker.join();
        }
    }

    ~NanosleepTimer()
    {
        stop();
    }

private:
    std::atomic<bool> active{false};
    std::thread worker;
    std::mutex callback_mutex;

    static void add_duration(timespec& ts, Duration delta)
    {
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
        ts.tv_sec += nanos / 1000000000LL;
        ts.tv_nsec += nanos % 1000000000LL;
        if (ts.tv_nsec >= 1000000000LL) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000LL;
        }
    }
};

#endif // W2RP_NanosleepTimer_h