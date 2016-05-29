#pragma once

#include "asio.hh"
#include "address.hh"
#include "socket.hh"

#include <deque>
#include <iostream>

namespace asio {
    class Message {
    public:
        Message() = default;
        Message(std::vector<Byte> data, Address address) : data(data), address(address) {}

        std::vector<Byte> data;
        Address address;
    };

    class UDPSocket : public IStream<Message>, public Socket {
    private:
        bool listening = false;

        std::deque<Message> write_buffer;
        std::deque<Message> read_buffer;

        std::size_t max_packet_size = 4096;

        Address::Family _family = Address::Family::Inet4;   // FIXME: statically put
    public:
        UDPSocket(Driver &driver);

        UDPSocket(Driver &driver, Address::Family family);

        ~UDPSocket();

        void bind(std::string host, Port port);
        void bind(std::string host, std::string service);
        void bind();

        void send(std::vector<Message> msg);

        std::vector<Message> receive(std::size_t size);

        std::size_t get_input_buffer_size();
        std::size_t get_output_buffer_size();

    protected:
        void setup_poller(bool modify);
        virtual void poller(IPoller *object);
        virtual void notify(std::vector<Event> events);

        virtual void quit_socket();
        void init_socket();
    };
}