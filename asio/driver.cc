#include "driver.hh"
#include "socket.hh"

namespace asio {
    void Driver::work() {
        if(working) {
            std::runtime_error("Driver is working right now!");
        }
        working = true;
        while(working) {
            for(auto conn : connections) {
                conn->trigger_ready();
            }
            if(alarm->empty() && connections.empty()) break;
            auto timeout = alarm->sleep_time(max_sleep_time);
            poller->wait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
            alarm->refresh();
        }
        working = false;
    }
}