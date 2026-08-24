#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/periodicEvent.hpp>
#include <w2rp/timer/timerManager.hpp>

#include <w2rp/log.hpp>

// #include <w2rp/w2rp-tp.h>

using namespace w2rp;

int send_counter;

bool handler()
{
    // tracepoint(w2rp_trace, tracepoint_writer_int, "SAMPLE_START, : ", ++send_counter);
    logInfo("Hello from handler!")
    return true;
}

int main()
{
    // example using functions
    TimerManager timer_manager;

    logInfo("Hello from main")

    std::chrono::microseconds cycle(4000);

    PeriodicEvent timer(
        timer_manager, 
        cycle,
        handler,
        NANOSLEEP_TIMER,
        true,
        true
    );
    
    timer_manager.start();

    // sleep(1);

    // timer.cancel();

    // sleep(1);
    // logInfo("Restarting timer")
    // timer.restart();

    sleep(10);

    // while(true)
    // {

    // }

    logInfo("Exiting")

    return 0;
}