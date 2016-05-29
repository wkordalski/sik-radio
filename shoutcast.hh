#include "asio/tcp.hh"
#include "asio/processor.hh"


class ShoutCastClient {
    asio::Processor<asio::TCPSocket> *processor;
    std::shared_ptr<asio::TCPSocket> connection;
    std::string response_proto = "";
    int response_status = -1;
    std::map<std::string, std::string> response_headers;
    int metaint = 0;
    asio::Slot<> connection_established;
    asio::Slot<> connection_closed;
    std::string _title = "";
    bool paused = true;
public:
    ShoutCastClient(std::shared_ptr<asio::TCPSocket> connection, std::string path,
                    std::map<std::string, std::string> headers);

    ~ShoutCastClient();

    void pause() {
        paused = true;
    }
    void play() {
        paused = false;
    }
    std::string title() {
        return _title;
    }

private:
    void on_connect(std::vector<asio::Byte> query);
    void on_disconnect();
    void read_status_line(std::string status_line);
    void read_headers(std::string header_line);
    void skip_audio_data(std::vector<asio::Byte> data);
    void read_audio_data(std::vector<asio::Byte> data);
    void read_audio_data_only(std::vector<asio::Byte> data);
    void read_metadata_header(asio::Byte size);
    void read_metadata_data(std::vector<asio::Byte> data);
};