#pragma once

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <set>
#include <type_traits>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

#include "events.hh"


namespace asio {
    enum class Event {
        Input,
        Output,
        Error,
        RdHangUp
    };

    class Address;
    class Socket;

    class Driver;

    class IPoller {
    public:
        virtual ~IPoller() {};

        virtual void add(std::shared_ptr<Socket> connection, std::initializer_list<Event> events) = 0;
        virtual void modify(std::shared_ptr<Socket> connection, std::initializer_list<Event> events) = 0;
        virtual void remove(std::shared_ptr<Socket> connection) = 0;

        virtual bool wait(std::chrono::milliseconds timeout) = 0;
    };

    template<typename T>
    class IStream {
    public:
        Signal<> on_data_received;
        Signal<> on_data_sent;
        Signal<> on_input_buffer_empty;
        Signal<> on_output_buffer_empty;

        virtual void send(std::vector<T> data) = 0;
        virtual std::vector<T> receive(std::size_t size) = 0;

        virtual std::size_t get_input_buffer_size() = 0;
        virtual std::size_t get_output_buffer_size() = 0;
    };



    template<typename T>
    class IListener {
    public:
        Signal<> on_connection_accepted;

        virtual std::shared_ptr<T> accept(Address &address) = 0;
    };

    using Byte = unsigned char;
    using Port = unsigned short;
}