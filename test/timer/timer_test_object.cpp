
#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/timerManager.hpp>

#include <w2rp/log.hpp>

using namespace w2rp;

int main()
{
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