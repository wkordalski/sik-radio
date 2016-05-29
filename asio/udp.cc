#include "udp.hh"

namespace asio {

    UDPSocket::UDPSocket(Driver &driver) : UDPSocket(driver, Address::Family::Inet4) {}

    UDPSocket::UDPSocket(Driver &driver, Address::Family family) : Socket(driver), _family(family) {}

    UDPSocket::~UDPSocket() {
        in_dtor = true;
        if(valid()) {
            this->close();
        }
    }

    void UDPSocket::init_socket() {
        int cfd = socket(Address::family_to_int(_family),
                         Address::socket_type_to_int(Address::SocketType::Datagram),
                         Address::protocol_to_int(Address::Protocol::UDP)
        );

        if(cfd < 0) {
            throw std::runtime_error("Error");
        }

        this->open(cfd);
        this->nonblocking(true);
    }

    void UDPSocket::bind(std::string host, Port port) {
        this->bind(host, std::to_string(port));
    }

    void UDPSocket::bind(std::string host, std::string service) {
        if(listening) throw std::runtime_error("Already bound");
        this->init_socket();
        auto addresses = Address::get_address(host, service, {Address::Family::Inet4},
                                              {Address::SocketType::Datagram}, {Address::Protocol::UDP}, true);

        for(auto a : addresses) {
            if(::bind(fd, &a.address(), a.address_length()) < 0) {
                continue;
            } else {
                break;
            }
        }
        listening = true;
    }

    void UDPSocket::bind() {
        this->init_socket();
        listening = true;
    }

    void UDPSocket::send(std::vector<Message> msg) {
        bool change_poller;
        if(write_buffer.empty() && !msg.empty()) {
            change_poller = true;
        }
        std::copy(msg.begin(), msg.end(), back_inserter(write_buffer));
        if(change_poller) {
            this->setup_poller(true);
        }
    }

    std::vector<Message> UDPSocket::receive(std::size_t size) {
        std::vector<Message> ret;
        auto count = std::min(size, read_buffer.size());
        ret.reserve(count);
        auto new_begin = read_buffer.begin() + count;
        std::copy(read_buffer.begin(), new_begin, back_inserter(ret));
        read_buffer.erase(read_buffer.begin(), new_begin);
        if(read_buffer.empty()) {
            this->on_input_buffer_empty();
        }
        return ret;
    }

    std::size_t UDPSocket::get_input_buffer_size() {
        return this->read_buffer.size();
    }

    std::size_t UDPSocket::get_output_buffer_size() {
        return this->write_buffer.size();
    }

    void UDPSocket::setup_poller(bool modify) {
        if(get_poller() != nullptr) {
            if (write_buffer.empty()) {
                if (modify) {
                    get_poller()->modify(shared_from_this(), {Event::Input, Event::Error});
                } else {
                    get_poller()->add(shared_from_this(), {Event::Input, Event::Error});
                }
            } else {
                if (modify) {
                    get_poller()->modify(shared_from_this(), {Event::Input, Event::Output, Event::Error});
                } else {
                    get_poller()->add(shared_from_this(), {Event::Input, Event::Output, Event::Error});
                }
            }
        }
    }
    void UDPSocket::poller(IPoller *object) {
        if(object == nullptr) {
            if(!in_dtor) get_poller()->remove(shared_from_this());
        } else {
            // always we can read data - because we are bound by default to some random port.
            setup_poller(false);
        }


    }
    void UDPSocket::notify(std::vector<Event> events) {
        // TODO - check for input messages or possibility of sending message
        auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };

        if(check(Event::Input)) {
            std::vector<Byte> buffer;
            buffer.resize(max_packet_size);
            struct sockaddr addr;
            socklen_t addrlen = sizeof(addr);
            addr.sa_family = Address::family_to_int(_family);
            ssize_t red = recvfrom(fd, buffer.data(), buffer.size(), 0, &addr, &addrlen);
            if (red < 0) {
                if (errno != EAGAIN) {
                    // TODO: on_error should be called
                    throw std::runtime_error("Reading failed - on_error should be called! - TODO");
                } else {
                    // Read nothing
                }
            }
            Message m;
            buffer.resize(std::size_t(red));     // cut to size
            m.data = buffer;
            m.address = Address(&addr, addrlen, _family, Address::SocketType::Datagram, Address::Protocol::UDP);
            read_buffer.push_back(m);
            this->on_data_received();
        }
        if(!write_buffer.empty() && check(Event::Output)) {
            Message &m = write_buffer.front();

            std::cerr << m.address.address_length() << std::endl;
            ssize_t written = sendto(fd, m.data.data(), m.data.size(), 0, &m.address.address(), m.address.address_length());
            if(written < 0) {
                if(errno != EAGAIN) {
                    // TODO: call on_error
                    throw std::runtime_error("Writing failed");
                } else {
                    // Written nothing
                }
            } else {
                write_buffer.pop_front();
                this->on_data_sent();

                if(write_buffer.empty()) {
                    this->setup_poller(true);
                    this->on_output_buffer_empty();
                }
            }
        }
    }

    void UDPSocket::quit_socket() {
        // call on_close?
        bool is_reset = true;
        this->close();
        this->socket_closed();
    }
}