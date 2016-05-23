#include "tcp.hh"

namespace asio {
    struct addrinfo * TCPConnection::get_address(std::string host, std::string service) {
        struct addrinfo hints, *result;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;

        if(::getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0) {
            return nullptr;
        }
        return result;
    }
    void TCPConnection::request_connect() {
        const struct addrinfo *addr = connect_current;
        if(addr == nullptr) {
            ::freeaddrinfo(connect_addresses);
            connect_addresses = connect_current = nullptr;
            this->on_connect_error(this);
            return;
        }

        int cfd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if(cfd == -1) {
            connect_current = addr->ai_next;
            request_connect();
            return;
        }
        this->open(cfd);
        this->nonblocking(true);
        if(::connect(fd, addr->ai_addr, addr->ai_addrlen) == -1) {
            if(errno != EINPROGRESS) {
                // error during connecting - try other one
                this->close();
                connect_current = addr->ai_next;
                request_connect();
                return;
            } else {
                // wait for events from poller
                this->set_poller_to_connect(true);
                return;
            }
        } else {
            // connected
            ::freeaddrinfo(connect_addresses);
            connect_addresses = connect_current = nullptr;
            this->peer_shutdowned = false;
            this->set_poller_to_transmission(true);
            this->on_connect(this);
            return;
        }
    }

    void TCPConnection::notify_connect(std::vector<Event> events) {
        auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };
        if(check(Event::Error)) {
            // Connect failed
            this->close();
            connect_current = connect_current->ai_next;
            request_connect();
            return;
        } else {
            if(check(Event::Output)) {
                // Connect succeeded
                ::freeaddrinfo(connect_addresses);
                connect_addresses = connect_current = nullptr;
                this->peer_shutdowned = false;
                this->set_poller_to_transmission(true);
                this->on_connect(this);
                return;
            }
        }
        assert(false);
    }

    void TCPConnection::notify_transmission(std::vector<Event> events) {
        auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };

        if(check(Event::Input)) {
            std::vector<Byte> buffer;
            buffer.resize(4096);
            int red = read(fd, buffer.data(), 4096);
            std::clog << "Red = " << red << std::endl;
            if(red < 0) {
                if(errno != EAGAIN) {
                    // TODO: on_error should be called
                    throw std::runtime_error("Reading failed - on_error should be called! - TODO");
                } else {
                    // Read nothing
                }
            } else if(red == 0) {
                this->peer_shutdowned = true;
                this->on_peer_shutdown(this);
                this->set_poller_to_transmission();
            } else {
                // read `red` bytes
                std::copy(buffer.begin(), buffer.begin() + red, back_inserter(read_buffer));
                this->on_data_received(this);
            }
        }
        if(!write_buffer.empty() && check(Event::Output)) {
            std::vector<Byte> buffer;
            auto count = std::min(std::size_t(4096), write_buffer.size());
            buffer.reserve(count);
            auto new_possible_begin = write_buffer.begin() + count;
            std::copy(write_buffer.begin(), new_possible_begin, back_inserter(buffer));
            int written = write(fd, buffer.data(), buffer.size());
            if(written < 0) {
                if(errno != EAGAIN) {
                    // TODO: call on_error
                    throw std::runtime_error("Writing failed");
                } else {
                    // Written nothing
                }
            } else if (written == 0) {
                // TODO ?
            } else {
                auto new_begin = write_buffer.begin() + written;
                write_buffer.erase(write_buffer.begin(), new_begin);
                this->on_data_sent(this);

                if(write_buffer.empty()) {
                    this->set_poller_to_transmission();
                    this->on_output_buffer_empty(this);
                }
            }
        }
    }

    void TCPConnection::set_poller_to_connect(bool modify) {
        if(get_poller() != nullptr) {
            if(modify) {
                get_poller()->modify(*this, {Event::Output, Event::Error});
            } else {
                get_poller()->add(*this, {Event::Output, Event::Error});
            }
        }
    }

    void TCPConnection::set_poller_to_transmission(bool modify) {
        if(get_poller() != nullptr) {
            if(this->peer_shutdowned) {
                if (write_buffer.empty()) {
                    if (modify) {
                        get_poller()->modify(*this, {Event::Error});
                    } else {
                        get_poller()->add(*this, {Event::Error});
                    }
                } else {
                    if (modify) {
                        get_poller()->modify(*this, {Event::Output, Event::Error});
                    } else {
                        get_poller()->add(*this, {Event::Output, Event::Error});
                    }
                }
            } else {
                if (write_buffer.empty()) {
                    if (modify) {
                        get_poller()->modify(*this, {Event::Input, Event::Error});
                    } else {
                        get_poller()->add(*this, {Event::Input, Event::Error});
                    }
                } else {
                    if (modify) {
                        get_poller()->modify(*this, {Event::Input, Event::Output, Event::Error});
                    } else {
                        get_poller()->add(*this, {Event::Input, Event::Output, Event::Error});
                    }
                }
            }
        }
    }


    void TCPConnection::connect(std::string host, std::string service) {
        if(connect_addresses != nullptr) {
            throw std::runtime_error("Socket already connecting...");
        }
        if(valid()) {
            throw std::runtime_error("Socket already connected...");
        }
        connect_addresses = connect_current = this->get_address(host, service);
        request_connect();
    }

    void TCPConnection::connect(std::string host, Port port) {
        this->connect(host, std::to_string(port));
    }

    void TCPConnection::disconnect(bool reset) {
        // TODO
    }

    void TCPConnection::poller(IPoller *object) {
        if(object == nullptr) {
            get_poller()->remove(*this);
        } else {
            if(connect_addresses != nullptr) {
                this->set_poller_to_connect(false);
            } else {
                this->set_poller_to_transmission(false);
            }
        }
    }

    void TCPConnection::notify(std::vector<Event> events) {
        if(connect_addresses != nullptr) {
            this->notify_connect(events);
        } else {
            this->notify_transmission(events);
        }
    }
}