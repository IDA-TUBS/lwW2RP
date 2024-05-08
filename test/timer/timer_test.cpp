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
    // example using functions
    TimerManager timer_manager;

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

    // example usage as part of a class, with methods as callbacks
    class timerObject
    {
        public:

        timerObject():
            serviceHandler(),
            cycle(500000),
            event_()
        {
            serviceHandler.start();

            event_ = new TimedEvent(
                serviceHandler,
                cycle,
                std::bind(&timerObject::callback, this)
            );

        };

        ~timerObject()
        {
            serviceHandler.stop();
        }

        void callback()
        {
            logInfo("Hello from timerObject callback!")
        };

        private:
        TimerManager serviceHandler;
        TimedEvent<>* event_;
        std::chrono::microseconds cycle;
    };

    timerObject myObject;
    
    while(true)
    {

    }

    return 0;
}