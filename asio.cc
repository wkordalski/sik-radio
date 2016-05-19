namespace asio {
    enum class Event {
        // TODO
    };

    class Driver {
    private:
        class NonmovableDriver {
            Driver & get() {
                // TODO
            }

            void set(Driver *driver) {
                // TODO
            }
        };

        NonmovableDriver *nmvdrv = nullptr;
    public:
        function<Driver&()> get_driver_getter() {
            NonmovableDriver *ptr = nmvdrv;
            return [=ptr]() { return ptr->get(); };
        }

        void add(FdConnection &connection);

        int get_new_object_id() {
            // TODO
        }

        void register_notify(int fd, function<void(Event event)> f) {
            // TODO
        }
        void unregister_notify(int fd) {
            // TODO
        }
    };

    class IObject {
    private:
        function<Driver&()> drv;
        int id = -1;
        int fd = -1;
    protected:
        int get_id() {
            // TODO
        }
        void set_id(int id) {
            // TODO
        }
    public:

        IObject(Driver &driver) : drv(driver.get_driver_getter()) {
            set_id(drv().get_new_object_id());
        }

        IObject(const IObject &orig) = delete;
        IObject(IObject &&orig) {
            drv = orig.drv;

            int id = orig.id;
            orig.set_id(-1);
            this->set_id(id);
        }

        ~IObject() {
            if(id != -1) {
                set_id(-1);
            }
        }

        IObject & operator= (IObject &rhs) = delete;
        IObject & operator= (IObject &&rhs) {
            this->set_id(-1);
            drv = orig.drv;

            int id = orig.id;
            orig.set_id(-1);
            this->set_id(id);
        }

        virtual void notify(Event e) = 0;
    };

    class IPoller {
        // TODO
    };

    class Epoll : public IPoller {
        int fd = -1;
        std::map<int, Descriptor*> descriptors;
        int max_events = 16;
        struct epoll_event *events;

    public:
        Epoll() {
            fd = epoll_create(0xCAFE);
            if(fd < 0) {
                throw std::runtime_error("Epoll creating failed.");
            }

            events = new struct epoll_event[max_events];
        }

        ~Epoll() {
            delete[] events;
        }

        virtual void add(FdConnection, unsigned int events) {
            epoll_event e;
            e.data.fd = descriptor.get_internal();
            e.events = events;

            if(epoll_ctl(fd, EPOLL_CTL_ADD, descriptor.get_internal(), &e) < 0) {
                throw std::runtime_error("Adding failed.");
            }
            descriptors[descriptor.get_internal()] = &descriptor;
        }

        void remove(Descriptor &descriptor) {
            if(!descriptor) return;
            auto descriptor_iterator = descriptors.find(descriptor.get_internal());

            if(descriptor_iterator == descriptors.end()) {
                throw std::runtime_error("Removing non-existing descriptor.");
            }

            if(epoll_ctl(fd, EPOLL_CTL_DEL, descriptor.get_internal(), nullptr) < 0) {
                throw std::runtime_error("Removing from epoll failed.");
            }
            descriptors.erase(descriptor_iterator);
        }

        void modify(Descriptor &descriptor, unsigned int events) {
            auto descriptor_iterator = descriptors.find(descriptor.get_internal());

            if(descriptor_iterator == descriptors.end()) {
                throw std::runtime_error("Modifying non-existing descriptor.");
            }


            epoll_event e;
            e.data.fd = descriptor.get_internal();
            e.events = events;

            if(epoll_ctl(fd, EPOLL_CTL_MOD, descriptor.get_internal(), &e) < 0) {
                throw std::runtime_error("EPoll modification failed.");
            }
        }

        bool wait(int timeout) {
            int got_events = epoll_wait(fd, events, max_events, timeout);

            for(int i = 0; i < got_events; i++) {
                int cfd = events[i].data.fd;
                auto descriptor_iterator = descriptors.find(cfd);
                if(descriptor_iterator != descriptors.end()) {
                    descriptor_iterator->second->on_notify(events[i].events);
                }
            }
            return true;
        }
    };


    template<typename T>
    class IStream {
    public:
        function<void()> on_stream_connected = [](){};
        function<void()> on_data_received = [](){};
        function<void()> on_data_sent = [](){};
        function<void()> on_input_buffer_empty = [](){};
        function<void()> on_output_buffer_empty = [](){};
        function<void()> on_stream_disconnect = [](){};
    };

    class FdConnection {
    protected:
        int fd = -1;

        FdConnection() {}
    };

    class IListener {
        // TODO
    };

    using Byte = unsigned char;
    using Message = vector<Byte>;
    using Port = unsigned short;

    class TCPConnection : public IStream<Byte> {
    public:
        TCPConnection(Driver &driver);

        void connect(string host, string service) {
            // TODO
        }

        void connect(string host, Port port) {
            // TODO
        }

        void disconnect() {
            // TODO
        }
    };

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
}