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
        Error
    };

    class FdConnection;

    class Driver;

    class IPoller {
    public:
        virtual ~IPoller() {};

        virtual void add(std::shared_ptr<FdConnection> connection, std::initializer_list<Event> events) = 0;
        virtual void modify(std::shared_ptr<FdConnection> connection, std::initializer_list<Event> events) = 0;
        virtual void remove(std::shared_ptr<FdConnection> connection) = 0;

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



    class IListener {
        // TODO
    };

    using Byte = unsigned char;
    using Message = std::vector<Byte>;
    using Port = unsigned short;

    /*
    class UPDConnection : public IStream<Message> {
        // TODO
    };

    class BasicConnectionListener : public IListener {
        // TODO
    };

    template<typename T, typename = enable_if<is_base_of<TCPConnection, T>>>
    class TCPListener : public BasicConnectionListener {
        // TODO
    };

    template<typename T, typename = enable_if<is_base_of<UDPConnection, T>>>
    class UDPListener : public BasicConnectionListener {
        // TODO
    };

    class ShoutcastConnection : public TCPConnection {
        // TODO
    };

    class MessageUDPConnection : public UDPConnection {
        // TODO
    };

    template<typename BaseStream>
    class ProcessedStream : public BaseStream {
        // TODO
    };

    class TelnetConnection : public ProcessedStreamConnection<TCPConnection> {
        // TODO
    };

    template<typename BaseConnection>
    class MessageTCPConnection : public ProcessedStreamConnection<BaseConnection> {
        // TODO
    };
     */
}