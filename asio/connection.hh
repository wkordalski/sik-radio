#pragma once

#include "driver.hh"

namespace asio {
    class FdConnection : public std::enable_shared_from_this<FdConnection> {
    protected:
        int fd = -1;
        IPoller *_poller = nullptr;
        std::function<void()> on_ready = [](){};
        bool ready_done = false;
        bool close_done = false;        // after close_done nothing can be done - connection removed from Driver
        Driver &driver;

    protected:
        void nonblocking(bool value) {
            if(!valid()) {
                throw std::runtime_error("Invalid descriptor.");
            }
            int flags = ::fcntl(fd, F_GETFL, 0);
            if(value) {
                fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            } else {
                fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
            }
        }

        bool nonblocking() {
            return (::fcntl(fd, F_GETFL, 0) & O_NONBLOCK);
        }

        void set_poller(IPoller *object) {
            if(valid()) {
                if(_poller != nullptr) poller(nullptr);
                _poller = object;
                if(object != nullptr) poller(object);
            } else {
                _poller = object;
            }
        }

        virtual void poller(IPoller *object) = 0;

        IPoller * get_poller() { return _poller; }

        void open(int fd) {
            if(valid()) {
                assert(false && "Already open");
            }
            this->fd = fd;
            if(_poller) {
                poller(_poller);
            }
        }

        void close() {
            if(valid()) {
                poller(nullptr);
                ::close(fd);
                fd = -1;
            }
        }

        virtual void reset_connection() = 0;

        void connection_closed() {
            auto self = shared_from_this();
            this->close_done = true;
            driver.remove_connection(self);
        }

    public:
        FdConnection(Driver &driver) : driver(driver) {}

        int get_descriptor() { return fd; }
        bool valid() { return fd >= 0 && !close_done; }
        bool is_closed() { return close_done; }

        virtual void notify(std::vector<Event> e) = 0;

        void trigger_ready() {
            if(!ready_done) {
                ready_done = true;
                this->on_ready();
                this->on_ready = [](){};
            }
        }

        void ready(std::function<void()> on_ready) {
            this->on_ready = on_ready;
        }

        friend class Driver;
    };
}