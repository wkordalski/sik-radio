#pragma once

#include "asio.hh"
#include "socket.hh"
#include "address.hh"

#include <algorithm>
#include <cstring>
#include <deque>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace asio {
    class TCPSocket : public IStream<Byte>, public Socket {
        std::vector<Address> connect_addresses;
        std::vector<Address>::iterator connect_current;
        bool connecting = false;

        bool peer_shutdowned = false;
        bool self_shutdowned = false;
        bool connected = false;

        std::deque<Byte> write_buffer;
        std::deque<Byte> read_buffer;
    public:
        Signal<> on_connect_error;
        Signal<> on_connect;
        Signal<> on_peer_shutdown;        // he won't write
        Signal<> on_self_shutdown;        // I won't write
        Signal<> on_connection_reset;
        Signal<> on_connection_close;
    private:
        void request_connect();

        void notify_connect(std::vector<Event> events);
        void notify_transmission(std::vector<Event> events);

        void set_poller_to_connect(bool modify = true);
        void set_poller_to_transmission(bool modify = true);
    public:
        TCPSocket(Driver &driver) : Socket(driver){
        }

        ~TCPSocket() {
            in_dtor = true;
            if(valid() && this->connected) {
                this->reset_connection();
            }
        }

        void connect(std::string host, std::string service);
        void connect(std::string host, Port port);

        void disconnect(bool reset = false);

        virtual void send(std::vector<Byte> data);
        virtual std::vector<Byte> receive(std::size_t size);

        virtual std::size_t get_input_buffer_size();
        virtual std::size_t get_output_buffer_size();

        void close_read() {
            ::shutdown(fd, 0);
            // TODO
        }

    protected:
        virtual void poller(IPoller *object);
        virtual void notify(std::vector<Event> events);
        virtual void reset_connection() {
            // call on_close?
            bool is_reset = !this->peer_shutdowned;
            this->close();
            if(!this->peer_shutdowned) {
                this->on_peer_shutdown();
            }
            if(!this->self_shutdowned) {
                this->on_self_shutdown();
            }
            if(is_reset) {
                this->on_connection_reset();
            } else {
                this->on_connection_close();
            }
            this->connection_closed();
        }
    };
}