#pragma once

#include "asio.hh"
#include "alarm.hh"

#include <utility>

namespace asio {
    template<typename Poller>
    class Driver {
        using clock = Alarm::clock;

        IPoller *poller;
        Alarm *alarm;
        std::set<FdConnection *> connections;
        bool working = false;

        const clock::duration max_sleep_time =
                std::chrono::duration_cast<clock::duration>(std::chrono::seconds(10));

    public:
        Driver() {
            poller = new Poller();
            alarm = new Alarm();
        }

        ~Driver() {
            delete poller;
            delete alarm;
        }

        template<typename T, typename = std::enable_if<std::is_base_of<FdConnection, T>::value>>
        void add_connection(T &&connection) {
            using rT = typename std::remove_reference<T>::type;
            rT *memory = new rT(std::move(connection));
            connections.insert(static_cast<FdConnection*>(memory));
            memory->set_poller(poller);
        }

        void work() {
            if(working) {
                std::runtime_error("Driver is working right now!");
            }
            working = true;
            while(working) {
                auto timeout = alarm->sleep_time(max_sleep_time);
                poller->wait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
                alarm->refresh();
            }
            working = false;
        }

        void stop() {
            working = false;
        }

        Alarm * get_alarm() { return alarm; }
    };
}