#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring>

namespace asio {

    class Address {
    public:
        enum class Family {
            Inet4
        };

        enum class SocketType {
            Stream,
            Datagram
        };

        enum class Protocol {
            TCP,
            UDP
        };
    private:
        struct sockaddr *addr;
        socklen_t len;
        Family _family;
        SocketType _socket_type;
        Protocol _protocol;
        bool valid = false;

    public:
        static Family int_to_family(int arg) {
            if(arg == AF_INET) return Family::Inet4;
            throw std::runtime_error("Wrong value");
        }

        static SocketType int_to_socket_type(int arg) {
            if(arg == SOCK_STREAM) return SocketType::Stream;
            if(arg == SOCK_DGRAM) return SocketType::Datagram;
            throw std::runtime_error("Wrong value");
        }

        static Protocol int_to_protocol(int arg) {
            if(arg == IPPROTO_TCP) return Protocol::TCP;
            if(arg == IPPROTO_UDP) return Protocol::UDP;
            throw std::runtime_error("Wrong value");
        }

        static int family_to_int(Family f) {
            switch(f) {
                case Family::Inet4:
                    return AF_INET;
            }
        }

        static int socket_type_to_int(SocketType t) {
            switch(t) {
                case SocketType::Stream:
                    return SOCK_STREAM;
                case SocketType::Datagram:
                    return SOCK_DGRAM;
            }
        }

        static int protocol_to_int(Protocol p) {
            switch(p) {
                case Protocol::TCP:
                    return IPPROTO_TCP;
                case Protocol::UDP:
                    return IPPROTO_UDP;
            }
        }
    public:
        Address() : valid(false) {}

        Address(struct sockaddr *addr, socklen_t len, Family family, SocketType socket_type, Protocol protocol)
                : len(len), _family(family), _socket_type(socket_type), _protocol(protocol), valid(true) {
            this->addr = (struct sockaddr*)new char[len];
            memcpy(this->addr, addr, len);
        }

        Address(const Address &orig) : len(orig.len), _family(orig._family), _socket_type(orig._socket_type),
                                       _protocol(orig._protocol), valid(orig.valid){
            if(orig.valid) {
                this->addr = (struct sockaddr *) new char[orig.len];
                memcpy(this->addr, orig.addr, len);
            }
        }

        Address(Address &&orig): len(orig.len), _family(orig._family), _socket_type(orig._socket_type),
                                 _protocol(orig._protocol), valid(orig.valid){
            if(orig.valid) {
                this->addr = orig.addr;
                orig.valid = false;
            }
        }

        ~Address() {
            if(valid) {
                delete[] this->addr;
            }
        }


        Address & operator= (const Address &orig) {
            len = orig.len;
            _family = orig._family;
            _socket_type = orig._socket_type;
            _protocol = orig._protocol;
            valid = orig.valid;

            if(orig.valid) {
                this->addr = (struct sockaddr *) new char[orig.len];
                memcpy(this->addr, orig.addr, len);
            }
            return *this;
        }

        Address & operator= (Address &&orig) {
            len = orig.len;
            _family = orig._family;
            _socket_type = orig._socket_type;
            _protocol = orig._protocol;
            valid = orig.valid;

            if(orig.valid) {
                this->addr = orig.addr;
                orig.valid = false;
            }
            return *this;
        }

        static std::vector<Address> get_address(std::string host, std::string service,
                                                std::vector<Family> family = {Family::Inet4},
                                                std::vector<SocketType> socket_type = {},
                                                std::vector<Protocol> protocol = {},
        bool passive = false) {
            int fml = 0;
            if(family.empty()) {
                fml = AF_UNSPEC;
            } else {
                for(auto f : family) {
                    fml |= family_to_int(f);
                }
            }

            int tpe = 0;
            for(auto t : socket_type) {
                tpe |= socket_type_to_int(t);
            }

            int prt = 0;
            for(auto p : protocol) {
                prt |= protocol_to_int(p);
            }

            struct addrinfo hints, *result;
            std::memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = fml;
            hints.ai_socktype = tpe;
            hints.ai_flags = (passive?AI_PASSIVE:0);
            hints.ai_protocol = prt;

            if(::getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0) {
                return {};
            }

            std::vector<Address> ret;
            for(auto i = result; i != nullptr; i = i->ai_next) {
                ret.push_back(Address(i->ai_addr, i->ai_addrlen,
                                      int_to_family(i->ai_family),
                                      int_to_socket_type(i->ai_socktype),
                                      int_to_protocol(i->ai_protocol)
                ));
            }

            ::freeaddrinfo(result);

            return ret;
        }

        Family family() { if(valid) return _family; else throw std::runtime_error("Error");}
        int family_as_int() { if(valid) return family_to_int(_family); else throw std::runtime_error("Error"); }

        SocketType socket_type() { if(valid) return _socket_type; else throw std::runtime_error("Error"); }
        int socket_type_as_int() { if(valid) return socket_type_to_int(_socket_type); else throw std::runtime_error("Error"); }

        Protocol protocol() { if(valid) return _protocol; else throw std::runtime_error("Error"); }
        int protocol_as_int() { if(valid) return protocol_to_int(_protocol); else throw std::runtime_error("Error"); }

        struct sockaddr & address() { if(valid) return *addr; else throw std::runtime_error("Error"); }
        socklen_t address_length() { if(valid) return len; else throw std::runtime_error("Error"); }

    };
}