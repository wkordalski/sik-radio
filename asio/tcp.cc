#include "tcp.hh"

namespace asio {
    void TCPSocket::request_connect() {
        if(!connecting || connect_current == connect_addresses.end()) {
            std::clog << "Connecting failed!" << std::endl;
            connect_addresses.clear();
            connecting = false;
            this->on_connect_error();
            return;
        }

        int cfd = ::socket(connect_current->family_as_int(), connect_current->socket_type_as_int(),
                           connect_current->protocol_as_int());
        if(cfd == -1) {
            connect_current++;
            request_connect();
            return;
        }
        this->open(cfd);
        this->nonblocking(true);
        if(::connect(fd, &connect_current->address(), connect_current->address_length()) == -1) {
            if(errno != EINPROGRESS) {
                // error during connecting - try other one
                this->close();
                connect_current++;
                request_connect();
                return;
            } else {
                // wait for events from poller
                this->set_poller_to_connect(true);
                return;
            }
        } else {
            // connected
            connect_addresses.clear();
            connecting = false;
            this->peer_shutdowned = false;
            this->set_poller_to_transmission(true);
            connected = true;
            this->on_connect();
            return;
        }
    }

    void TCPSocket::notify_connect(std::vector<Event> events) {
        auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };
        if(check(Event::Error)) {
            // Connect failed
            this->close();
            connect_current++;
            request_connect();
            return;
        } else {
            if(check(Event::Output)) {
                // Connect succeeded
                connect_addresses.clear();
                connecting = false;
                this->peer_shutdowned = false;
                this->set_poller_to_transmission(true);
                connected = true;
                this->on_connect();
                return;
            }
        }
        assert(false);
    }

    void TCPSocket::notify_transmission(std::vector<Event> events) {
        auto self = shared_from_this();
        auto check = [&events](Event e)->bool{ return std::find(events.begin(), events.end(), e) != events.end(); };

        if(check(Event::RdHangUp)) {
            // connection reset by peer
            this->close();
            if(!this->peer_shutdowned) {
                this->on_peer_shutdown();
            }
            if(!this->self_shutdowned) {
                this->on_self_shutdown();
            }
            this->on_connection_reset();
            this->socket_closed();
            return;
        }

        if(check(Event::Input)) {
            std::vector<Byte> buffer;
            buffer.resize(4096);
            int red = read(fd, buffer.data(), 4096);
            if(red < 0) {
                if(errno != EAGAIN) {
                    // TODO: on_error should be called
                    throw std::runtime_error(strerror(errno));
                } else {
                    // Read nothing
                }
            } else if(red == 0) {
                this->peer_shutdowned = true;
                this->on_peer_shutdown();
                this->set_poller_to_transmission();
            } else {
                // read `red` bytes
                std::copy(buffer.begin(), buffer.begin() + red, back_inserter(read_buffer));
                this->on_data_received();
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
                this->on_data_sent();

                if(write_buffer.empty()) {
                    this->set_poller_to_transmission();
                    this->on_output_buffer_empty();
                }
            }
        }
    }

    void TCPSocket::set_poller_to_connect(bool modify) {
        if(get_poller() != nullptr) {
            if(modify) {
                get_poller()->modify(shared_from_this(), {Event::Output, Event::Error});
            } else {
                get_poller()->add(shared_from_this(), {Event::Output, Event::Error});
            }
        }
    }

    void TCPSocket::set_poller_to_transmission(bool modify) {
        if(get_poller() != nullptr) {
            if(this->peer_shutdowned) {
                if (write_buffer.empty()) {
                    if (modify) {
                        get_poller()->modify(shared_from_this(), {Event::Error, Event::RdHangUp});
                    } else {
                        get_poller()->add(shared_from_this(), {Event::Error, Event::RdHangUp});
                    }
                } else {
                    if (modify) {
                        get_poller()->modify(shared_from_this(), {Event::Output, Event::Error, Event::RdHangUp});
                    } else {
                        get_poller()->add(shared_from_this(), {Event::Output, Event::Error, Event::RdHangUp});
                    }
                }
            } else {
                if (write_buffer.empty()) {
                    if (modify) {
                        get_poller()->modify(shared_from_this(), {Event::Input, Event::Error, Event::RdHangUp});
                    } else {
                        get_poller()->add(shared_from_this(), {Event::Input, Event::Error, Event::RdHangUp});
                    }
                } else {
                    if (modify) {
                        get_poller()->modify(shared_from_this(), {Event::Input, Event::Output, Event::Error, Event::RdHangUp});
                    } else {
                        get_poller()->add(shared_from_this(), {Event::Input, Event::Output, Event::Error, Event::RdHangUp});
                    }
                }
            }
        }
    }


    void TCPSocket::connect(std::string host, std::string service) {
        if(connecting) {
            throw std::runtime_error("Socket already connecting...");
        }
        if(valid()) {
            throw std::runtime_error("Socket already connected...");
        }
        connect_addresses = Address::get_address(host, service, {Address::Family::Inet4},
                                                 {Address::SocketType::Stream}, {Address::Protocol::TCP });
        connect_current = connect_addresses.begin();
        connecting = true;
        request_connect();
    }

    void TCPSocket::connect(std::string host, Port port) {
        this->connect(host, std::to_string(port));
    }

    void TCPSocket::disconnect(bool reset) {
        if(reset) {
            // Do simple "close"
            this->quit_socket();
        } else {
            if(!self_shutdowned) {
                if (peer_shutdowned) {
                    ::shutdown(fd, 1);
                    this->on_self_shutdown();
                    this->close();
                    this->on_connection_close();
                    this->connected = false;
                    this->socket_closed();
                } else {
                    // Make shutdown only - receiving still possible
                    if (valid()) {
                        ::shutdown(fd, 1);
                        self_shutdowned = true;
                        this->on_self_shutdown();
                    }
                }
            }
        }
    }

    void TCPSocket::send(std::vector<Byte> data) {
        bool change_poller;
        if(write_buffer.empty() && !data.empty()) {
            change_poller = true;
        }
        std::copy(data.begin(), data.end(), back_inserter(write_buffer));
        this->set_poller_to_transmission();
    }

    std::vector<Byte> TCPSocket::receive(std::size_t size) {
        std::vector<Byte> ret;
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

    std::size_t TCPSocket::get_input_buffer_size() {
        return this->read_buffer.size();
    }

    std::size_t TCPSocket::get_output_buffer_size() {
        return this->write_buffer.size();
    }

    void TCPSocket::poller(IPoller *object) {
        if(object == nullptr) {
            if(!in_dtor) get_poller()->remove(shared_from_this());
        } else {
            if(connecting) {
                this->set_poller_to_connect(false);
            } else {
                this->set_poller_to_transmission(false);
            }
        }
    }

    void TCPSocket::notify(std::vector<Event> events) {
        if(connecting) {
            this->notify_connect(events);
        } else {
            this->notify_transmission(events);
        }
    }
}