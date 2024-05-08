#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/timerManager.hpp>

#include <w2rp/log.hpp>

using namespace w2rp;


void handler()
{
    logInfo("Hello from handler!")
}

void trigger_handler()
{
    logInfo("Hello from handler2!")
}

int main()
{
    TimerManager timer_manager;

    TimedEvent<>* new_event;

    std::chrono::microseconds cycle(500000);

    TimedEvent<>* new_event;
    new_event = new TimedEvent(
        timer_manager, 
        cycle,
        handler 
    );

    TimedEvent timer(
        timer_manager, 
        cycle,
        handler
    );
    
    std::chrono::system_clock::time_point trigger = std::chrono::system_clock::now() + std::chrono::seconds(1);

    TimedEvent timer2(
        timer_manager, 
        trigger,
        trigger_handler
    );
    
    new_event = new TimedEvent(
        timer_manager, 
        cycle,
        handler 
    );

    timer_manager.start();
    
    while(true)
    {

    }

    return 0;
}