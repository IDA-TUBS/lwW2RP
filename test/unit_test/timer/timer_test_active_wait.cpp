#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/periodicEvent.hpp>
#include <w2rp/timer/timerManager.hpp>

#include <w2rp/log.hpp>

#include <w2rp/w2rp-tp.h>

using namespace w2rp;

int send_counter;

bool handler()
{
    tracepoint(w2rp_trace, tracepoint_writer_int, "SAMPLE_START, : ", ++send_counter);
    // logInfo("Hello from handler!")
    return true;
}



int main()
{
    // example using functions
    TimerManager timer_manager;

    logInfo("Hello from main")

    // std::chrono::microseconds cycle(500000);
    std::chrono::microseconds cycle(1000);

    PeriodicEvent timer(
        timer_manager, 
        cycle,
        handler,
        ACTIVEWAIT_TIMER,
        true
    );
    
    timer_manager.start();

    while(true)
    {

    }

    return 0;
}