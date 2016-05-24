#pragma once

#include "asio.hh"

#include <initializer_list>
#include <map>

#include <sys/epoll.h>

namespace asio {

    class Epoll : public IPoller {
        int fd = -1;
        std::map<int, std::shared_ptr<FdConnection>> connections;
        int max_events = 16;
        struct epoll_event *events;

    public:
        Epoll();

        virtual ~Epoll();

        virtual void add(std::shared_ptr<FdConnection> connection, std::initializer_list<Event> events);
        virtual void modify(std::shared_ptr<FdConnection> connection, std::initializer_list<Event> events);
        virtual void remove(std::shared_ptr<FdConnection> connection);

        // false -> timeout exceeded
        virtual bool wait(std::chrono::milliseconds timeout);
    };
}