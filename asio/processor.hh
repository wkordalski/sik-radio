#include "asio.hh"

namespace asio {
    class Processor {
    public:
        class Task {
        public:
            // Interface
            virtual void on_byte_received(Processor *p, Byte b) = 0;
        };
    private:
        IStream<Byte> *stream;
        Task *_task;

        void data_received_handler(IStream<Byte> *stream) {
            this->stream = stream;
            while(_task != nullptr && stream->get_input_buffer_size() > 0) {
                auto data = stream->receive(1);
                _task->on_byte_received(this, data[0]);
            }
        }

    public:
        Processor(IStream<Byte> *stream) : stream(stream) {
            stream->on_data_received = std::bind(&Processor::data_received_handler, this, std::placeholders::_1);
        }

        ~Processor() {
            stream->on_data_received = [](IStream<Byte> *stream) {};
            if(this->_task) finish();
        }

        void task(Task *tk) {
            this->_task = tk;
            data_received_handler(stream);
        }

        void finish() {
            this->_task = nullptr;
        }

        IStream<Byte> * get_stream() { return stream; }
    };

    namespace tasks {
        class ReadLineCRLF : public Processor::Task {
            std::string buffer = "";
            std::function<void(Processor*,std::string)> action;
        public:
            ReadLineCRLF(std::function<void(Processor*,std::string)> action) : action(action) {}
            virtual void on_byte_received(Processor *p, Byte b) {
                if(b == '\n' && buffer.size() > 0 && *(buffer.rbegin()) == '\r') {
                    buffer.erase(buffer.size() - 1);
                    std::string tmp = "";
                    std::swap(tmp, buffer);
                    p->finish();
                    action(p, std::move(tmp));
                    delete this;
                } else {
                    buffer += char(b);
                }
            }
        };

        class ReadChunk : public Processor::Task {
            std::vector<Byte> buffer;
            std::function<void(std::vector<Byte> data)> action;
            std::size_t chunk_size = 0;
        public:
            ReadChunk(std::size_t chunk_size, std::function<void(std::vector<Byte>)> action)
                    : action(action), chunk_size(chunk_size) {
                assert(chunk_size > 0);
            }

            virtual void on_byte_received(Processor *p, Byte b) {
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

        class ReadByte : public Processor::Task {
            std::function<void(Byte)> action;
        public:
            ReadByte(std::function<void(Byte)> action) : action(action) {}

            virtual void on_byte_received(Processor *p, Byte b) {
                p->finish();
                action(b);
                delete this;
            }
        };

        class InfiniteStdout : public Processor::Task {
        public:
            virtual void on_byte_received(Processor *p, Byte b) {
                write(1, &b, 1);
            }
        };
    }
}