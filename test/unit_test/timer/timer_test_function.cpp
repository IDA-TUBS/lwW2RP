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

// void trigger_handler()
// {
//     logInfo("Hello from handler2!")
// }

int main()
{
    // example using functions
    TimerManager timer_manager;

    // std::chrono::microseconds cycle(500000);
    std::chrono::microseconds cycle2(1000);

    // PeriodicEvent<>* new_event;
    // new_event = new PeriodicEvent(
    //     timer_manager, 
    //     cycle,
    //     handler 
    // );

    PeriodicEvent timer(
        timer_manager, 
        cycle2,
        handler
    );
    
    // std::chrono::system_clock::time_point trigger = std::chrono::system_clock::now() + std::chrono::seconds(1);

    // TimedEvent timer2(
    //     timer_manager, 
    //     trigger,
    //     trigger_handler
    // );
    
    timer_manager.start();

    while(true)
    {

    }

    return 0;
}