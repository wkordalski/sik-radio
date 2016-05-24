#include "driver.hh"
#include "connection.hh"

namespace asio {
    void Driver::work() {
        for(auto conn : connections) {
            conn->trigger_ready();
        }
        if(working) {
            std::runtime_error("Driver is working right now!");
        }
        working = true;
        while(working) {
            if(alarm->empty() && connections.empty()) break;
            auto timeout = alarm->sleep_time(max_sleep_time);
            poller->wait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
            alarm->refresh();
        }
        working = false;
    }
}