#pragma once

#include "asio.hh"
#include "alarm.hh"

#include <memory>
#include <utility>

namespace asio {
    class Driver {
        using clock = Alarm::clock;

        IPoller *poller;
        Alarm *alarm;
        std::set<std::shared_ptr<Socket>> connections;
        bool working = false;

        const clock::duration max_sleep_time =
                std::chrono::duration_cast<clock::duration>(std::chrono::seconds(10));

    public:
        Driver(IPoller *poller) {
            this->poller = poller;
            alarm = new Alarm();
        }

        ~Driver() {
            delete poller;
            delete alarm;
        }

        template<typename T, typename = std::enable_if<std::is_base_of<Socket, T>::value>>
        std::shared_ptr<T> make_socket() {
            auto ptr = std::shared_ptr<T>(new T(*this));
            connections.insert(ptr);
            ptr->set_poller(poller);
            return ptr;
        }

        template<typename T, typename = std::enable_if<std::is_base_of<Socket, T>::value>>
        void remove_socket(std::shared_ptr<T> connection) {
            auto iter = connections.find(connection);
            if(iter != connections.end()) {
                connections.erase(iter);
            }
        }

        void work();

        void stop() {
            working = false;
        }

        Alarm * get_alarm() { return alarm; }
    };
}