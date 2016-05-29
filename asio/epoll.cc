#include "epoll.hh"

#include <sys/epoll.h>
#include <iostream>

#include "socket.hh"

namespace {
    unsigned int epoll_event_type_from_event(asio::Event event) {
        switch(event) {
            case asio::Event::Input:
                return EPOLLIN;
            case asio::Event::Output:
                return EPOLLOUT;
            case asio::Event::Error:
                return EPOLLERR;
            case asio::Event::RdHangUp:
                return EPOLLRDHUP;
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
        if(ev & EPOLLRDHUP) {
            ret.push_back(asio::Event::RdHangUp);
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

    void Epoll::add(std::shared_ptr<Socket> connection, std::initializer_list<Event> events) {
        epoll_event e;
        int cfd = connection->get_descriptor();
        e.data.fd = cfd;
        e.events = epoll_event_type_from_events(events);

        if(epoll_ctl(fd, EPOLL_CTL_ADD, cfd, &e) < 0) {
            throw std::runtime_error("Adding failed.");
        }
        connections[cfd] = connection;
    }

    void Epoll::modify(std::shared_ptr<Socket> connection, std::initializer_list<Event> events) {
        int cfd = connection->get_descriptor();
        auto connections_iterator = connections.find(cfd);

        if(connections_iterator == connections.end()) {
            throw std::runtime_error("Modifying non-existing descriptor.");
        }



        epoll_event e;
        e.data.fd = cfd;
        e.events = epoll_event_type_from_events(events);

        if(epoll_ctl(fd, EPOLL_CTL_MOD, cfd, &e) < 0) {
            throw std::runtime_error("EPoll modification failed.");
        }
    }

    void Epoll::remove(std::shared_ptr<Socket> connection) {
        if(!connection->valid()) return;
        int cfd = connection->get_descriptor();
        auto connections_iterator = connections.find(cfd);

        if(connections_iterator == connections.end()) {
            throw std::runtime_error("Removing non-existing descriptor.");
        }

        if(epoll_ctl(fd, EPOLL_CTL_DEL, cfd, nullptr) < 0) {
            throw std::runtime_error("Removing from epoll failed.");
        }
        connections.erase(connections_iterator);
    }


    bool Epoll::wait(std::chrono::milliseconds timeout) {
        int got_events = epoll_wait(fd, events, max_events, timeout.count());

        if(got_events == 0) {
            return false;
        }


        for(int i = 0; i < got_events; i++) {
            int cfd = events[i].data.fd;
            auto connections_iterator = connections.find(cfd);
            if(connections_iterator != connections.end()) {
                connections_iterator->second->notify(events_from_epoll_event_type_for_connections(events[i].events));
            } else {
                std::clog << "Unfound" << std::endl;
            }
        }

        return true;
    }
}