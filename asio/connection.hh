#pragma once

#include "driver.hh"

namespace asio {
    class FdConnection {
    protected:
        int fd = -1;
        IPoller *_poller = nullptr;

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

    public:
        int get_descriptor() { return fd; }
        bool valid() { return fd >= 0; }

        virtual void notify(std::vector<Event> e) = 0;

        template<typename T> friend class Driver;
    };
}