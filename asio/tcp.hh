#pragma once

#include "asio.hh"
#include "connection.hh"

#include <algorithm>
#include <cstring>
#include <deque>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace asio {
    class TCPConnection : public IStream<Byte>, public FdConnection {
        struct addrinfo *connect_addresses = nullptr;
        struct addrinfo *connect_current = nullptr;

        bool peer_shutdowned = false;

        std::deque<Byte> write_buffer;
        std::deque<Byte> read_buffer;
    public:
        std::function<void(TCPConnection*)> on_connect_error = [](TCPConnection*){};
        std::function<void(TCPConnection*)> on_connect = [](TCPConnection*){};
        std::function<void(TCPConnection*)> on_peer_shutdown = [](TCPConnection*){};
    private:
        struct addrinfo * get_address(std::string host, std::string service);
        void request_connect();

        void notify_connect(std::vector<Event> events);
        void notify_transmission(std::vector<Event> events);

        void set_poller_to_connect(bool modify = true);
        void set_poller_to_transmission(bool modify = true);
    public:
        TCPConnection() {
        }

        void connect(std::string host, std::string service);
        void connect(std::string host, Port port);

        void disconnect(bool reset = false);

        virtual void send(std::vector<Byte> data) {
            bool change_poller;
            if(write_buffer.empty() && !data.empty()) {
                change_poller = true;
            }
            std::copy(data.begin(), data.end(), back_inserter(write_buffer));
            this->set_poller_to_transmission();
        }
        virtual std::vector<Byte> receive(std::size_t size) {
            std::vector<Byte> ret;
            auto count = std::min(size, read_buffer.size());
            ret.reserve(count);
            auto new_begin = read_buffer.begin() + count;
            std::copy(read_buffer.begin(), new_begin, back_inserter(ret));
            read_buffer.erase(read_buffer.begin(), new_begin);
            if(read_buffer.empty()) {
                this->on_input_buffer_empty(this);
            }
            return ret;
        }

    protected:
        virtual void poller(IPoller *object);
        virtual void notify(std::vector<Event> events);
    };
}