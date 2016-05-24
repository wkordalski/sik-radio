#include "asio.hh"

namespace asio {
    class IProcessor {
    public:
        virtual void finish() = 0;
    };

    class Task {
    public:
        // Interface
        virtual void on_byte_received(IProcessor *p, Byte b) = 0;
    };

    template<typename T, typename = std::enable_if<std::is_base_of<IStream<Byte>, T>::value>>
    class Processor : public IProcessor {
    public:
        Signal<> on_connection_closed;  // for read
    private:
        std::shared_ptr<T> stream;
        Task *_task;
        Slot<> data_received;
        Slot<> connection_closed;

        void data_received_handler() {
            while(_task != nullptr && stream->get_input_buffer_size() > 0) {
                auto data = stream->receive(1);
                _task->on_byte_received(this, data[0]);
            }
        }

        void connection_closed_handler() {
            stream.reset();
            _task = nullptr;
        }
    public:
        Processor(std::shared_ptr<T> stream)
                : stream(stream), data_received(std::bind(&Processor::data_received_handler, this)),
                  connection_closed(std::bind(&Processor::connection_closed_handler, this)) {
            stream->on_data_received.add(data_received);
            stream->on_peer_shutdown.add(connection_closed);
        }

        ~Processor() {
            stream->on_data_received = [](IStream<Byte> *stream) {};
            if(this->_task) finish();
        }

        void task(Task *tk) {
            this->_task = tk;
            data_received_handler();
        }

        virtual void finish() {
            this->_task = nullptr;
        }

        IStream<Byte> * get_stream() { return stream; }
    };

    namespace tasks {
        class ReadLineCRLF : public Task {
            std::string buffer = "";
            std::function<void(std::string)> action;
        public:
            ReadLineCRLF(std::function<void(std::string)> action) : action(action) {}
            virtual void on_byte_received(IProcessor *p, Byte b) {
                if(b == '\n' && buffer.size() > 0 && *(buffer.rbegin()) == '\r') {
                    buffer.erase(buffer.size() - 1);
                    std::string tmp = "";
                    std::swap(tmp, buffer);
                    p->finish();
                    action(std::move(tmp));
                    delete this;
                } else {
                    buffer += char(b);
                }
            }
        };

        class ReadChunk : public Task {
            std::vector<Byte> buffer;
            std::function<void(std::vector<Byte> data)> action;
            std::size_t chunk_size = 0;
        public:
            ReadChunk(std::size_t chunk_size, std::function<void(std::vector<Byte>)> action)
                    : action(action), chunk_size(chunk_size) {
                assert(chunk_size > 0);
            }

            virtual void on_byte_received(IProcessor *p, Byte b) {
                buffer.push_back(b);
                assert(buffer.size() <= chunk_size);
                if(buffer.size() == chunk_size) {
                    std::vector<Byte> tmp;
                    std::swap(tmp, buffer);
                    p->finish();
                    action(tmp);
                    delete this;
                }
            }
        };

        class ReadByte : public Task {
            std::function<void(Byte)> action;
        public:
            ReadByte(std::function<void(Byte)> action) : action(action) {}

            virtual void on_byte_received(IProcessor *p, Byte b) {
                p->finish();
                action(b);
                delete this;
            }
        };

        class InfiniteStdout : public Task {
        public:
            virtual void on_byte_received(IProcessor *p, Byte b) {
                write(1, &b, 1);
            }
        };
    }
}