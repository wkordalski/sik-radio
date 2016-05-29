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

        void setup_socket_from_listener(int fd) {
            this->open(fd);
            this->nonblocking(true);
            connected = true;
            this->on_connect();
        }
    public:
        TCPSocket(Driver &driver) : Socket(driver){
        }

        ~TCPSocket() {
            in_dtor = true;
            if(valid() && this->connected) {
                this->quit_socket();
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
        virtual void quit_socket() {
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
            this->socket_closed();
        }

        template<typename T, typename U> friend class TCPListener;
    };

    template<typename T, typename = std::enable_if<std::is_base_of<TCPSocket, T>::value>>
    class TCPListener : public IListener<T>, public Socket {
    public:
        Signal<> on_close_listener;
        Address::Family _family = Address::Family::Inet4;
        bool listening = false;
        int max_queue = 16;

    public:
        TCPListener(Driver &driver) : Socket(driver) {}
        ~TCPListener() {
            in_dtor = true;
            // TODO
        }

        void listen(std::string host, std::string service) {
            if(listening) throw std::runtime_error("Already listening");
            auto addresses = Address::get_address(host, service, {Address::Family::Inet4},
                                                  {Address::SocketType::Stream}, {Address::Protocol::TCP}, true);

            for(auto a : addresses) {
                this->init_socket();
                if(::bind(fd, &a.address(), a.address_length()) < 0) {
                    this->close();
                    continue;
                } else {
                    if(::listen(fd, max_queue) < 0) {
                        this->close();
                        continue;
                    } else {
                        listening = true;
                    }
                }
            }
            if(listening == false) {
                throw std::runtime_error(strerror(errno));
            }
        }

        void listen(std::string host, Port port) {
            this->listen(host, std::to_string(port));
        }

        virtual std::shared_ptr<T> accept(Address &address) {
            struct sockaddr addr;
            socklen_t addrlen;
            addr.sa_family = Address::family_to_int(Address::Family::Inet4);
            addrlen = sizeof(addr);
            int cfd = ::accept(fd, &addr, &addrlen);
            if(cfd < 0) {
                if(errno != EAGAIN && errno != EWOULDBLOCK) {
                    // error
                    throw std::runtime_error("Error!");
                } else {
                    return std::shared_ptr<T>();
                }
            }
            address = Address(&addr, addrlen, Address::Family::Inet4, Address::SocketType::Stream, Address::Protocol::TCP);
            auto C = driver.make_socket<T>();
            std::cerr << cfd << std::endl;
            C->setup_socket_from_listener(cfd);
            return C;
        }

    protected:

        void init_socket() {
            int cfd = socket(Address::family_to_int(_family),
                             Address::socket_type_to_int(Address::SocketType::Stream),
                             Address::protocol_to_int(Address::Protocol::TCP)
            );

            if(cfd < 0) {
                throw std::runtime_error("Error");
            }

            this->open(cfd);
            this->nonblocking(true);
        }
        virtual void quit_socket() {
            // call on_close?
            this->close();
            this->on_close_listener();
            this->socket_closed();
        }
        virtual void notify(std::vector<Event> events) {
            auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };

            if(check(Event::Input)) {
                this->on_connection_accepted();
            }
        }
        virtual void poller(IPoller *object) {
            if(object == nullptr) {
                if(!in_dtor) get_poller()->remove(shared_from_this());
            } else {
                get_poller()->add(shared_from_this(), {Event::Input});
            }
        }
    };
}