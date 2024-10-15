
#include <w2rp/timer/timedEvent.hpp>
#include <w2rp/timer/periodicEvent.hpp>
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
            trigger_tp(std::chrono::system_clock::now() + std::chrono::seconds(1)),
            t_event_(),
            p_event_()
        {
            serviceHandler.start();

            t_event_ = new TimedEvent(
                serviceHandler,
                trigger_tp,
                std::bind(&timerObject::t_callback, this)
            );

            p_event_ = new PeriodicEvent(
                serviceHandler,
                cycle,
                std::bind(&timerObject::p_callback, this)
            );

        };

        ~timerObject()
        {
            serviceHandler.stop();
        }

        void t_callback()
        {
            logInfo("Hello from timerObject callback!")
        };

        bool p_callback()
        {
            logInfo("Hello from periodic timerObject callback!")
            return true;
        };

        void cancel()
        {
            p_event_->cancel();
        };

        void start()
        {
            p_event_->restart();
        };

        private:
        TimerManager serviceHandler;
        TimedEvent<>* t_event_;
        PeriodicEvent<>* p_event_;
        std::chrono::microseconds cycle;
        std::chrono::system_clock::time_point trigger_tp;
    };

    timerObject myObject;
    
    sleep(2);

    myObject.cancel();

    sleep(2);

    myObject.start();

    while(true)
    {

    }

    return 0;
}