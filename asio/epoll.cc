#include "epoll.hh"

#include <sys/epoll.h>
#include <iostream>

#include "connection.hh"

namespace {
    unsigned int epoll_event_type_from_event(asio::Event event) {
        switch(event) {
            case asio::Event::Input:
                return EPOLLIN;
            case asio::Event::Output:
                return EPOLLOUT;
            case asio::Event::Error:
                return EPOLLERR;
        }
    }

    unsigned int epoll_event_type_from_events(std::initializer_list<asio::Event> events) {
        unsigned int ret = 0;
        for(asio::Event e : events) {
            ret |= epoll_event_type_from_event(e);
        }
        return ret;
    }

    std::vector<asio::Event> events_from_epoll_event_type_for_connections(unsigned int ev) {
        std::vector<asio::Event> ret;
        if(ev & EPOLLIN) {
            ret.push_back(asio::Event::Input);
        }
        if(ev & EPOLLOUT) {
            ret.push_back(asio::Event::Output);
        }
        if(ev & EPOLLERR) {
            ret.push_back(asio::Event::Error);
        }
        return ret;
    }
}

namespace asio {
     Epoll::Epoll() {
        fd = epoll_create(0xCAFE);
        if(fd < 0) {
            throw std::runtime_error("Epoll creating failed.");
        }

        events = new struct ::epoll_event[max_events];
    }

    Epoll::~Epoll() {
        delete[] events;
    }

    void Epoll::add(FdConnection &connection, std::initializer_list<Event> events) {
        epoll_event e;
        e.data.fd = connection.get_descriptor();
        e.events = epoll_event_type_from_events(events);

        std::clog << "Adding " << e.data.fd << "[" << e.events << "]" << std::endl;

        if(epoll_ctl(fd, EPOLL_CTL_ADD, connection.get_descriptor(), &e) < 0) {
            throw std::runtime_error("Adding failed.");
        }
        connections[connection.get_descriptor()] = &connection;
    }

    void Epoll::modify(FdConnection &connection, std::initializer_list<Event> events) {
        auto connections_iterator = connections.find(connection.get_descriptor());

        if(connections_iterator == connections.end()) {
            throw std::runtime_error("Modifying non-existing descriptor.");
        }



        epoll_event e;
        e.data.fd = connection.get_descriptor();
        e.events = epoll_event_type_from_events(events);

        std::clog << "Modify " << e.data.fd << "[" << e.events << "]" << std::endl;

        if(epoll_ctl(fd, EPOLL_CTL_MOD, connection.get_descriptor(), &e) < 0) {
            throw std::runtime_error("EPoll modification failed.");
        }
    }

    void Epoll::remove(FdConnection &connection) {
        if(!connection.valid()) return;
        auto connections_iterator = connections.find(connection.get_descriptor());

        if(connections_iterator == connections.end()) {
            throw std::runtime_error("Removing non-existing descriptor.");
        }

        std::clog << "Removing " << connection.get_descriptor() << std::endl;

        if(epoll_ctl(fd, EPOLL_CTL_DEL, connection.get_descriptor(), nullptr) < 0) {
            throw std::runtime_error("Removing from epoll failed.");
        }
        connections.erase(connections_iterator);
    }


    bool Epoll::wait(std::chrono::milliseconds timeout) {
        std::clog << " [" << timeout.count() << "] ";
        int got_events = epoll_wait(fd, events, max_events, timeout.count());

        std::clog << "Waiting " << got_events << " of " << this << std::endl;

        if(got_events == 0) {
            return false;
        }


        for(int i = 0; i < got_events; i++) {
            int cfd = events[i].data.fd;
            auto connections_iterator = connections.find(cfd);
            if(connections_iterator != connections.end()) {
                std::clog << "Events " << events[i].events << std::endl;
                connections_iterator->second->notify(events_from_epoll_event_type_for_connections(events[i].events));
            } else {
                std::clog << "Unfound" << std::endl;
            }
        }

        return true;
    }
}