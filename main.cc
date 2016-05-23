#include <iostream>

#include "asio/driver.hh"
#include "asio/epoll.hh"
#include "asio/tcp.hh"

using namespace std;

int main() {
    asio::Driver<asio::Epoll> D;
    asio::TCPConnection C;
    auto alarm = D.get_alarm();
    alarm->at(asio::Alarm::clock::now() + std::chrono::seconds(10), [&D](){D.stop();});
    C.on_connect = [&D](asio::TCPConnection *self){
        clog << "Connected." << endl;
        std::string request = "GET / HTTP/1.0\n\n";
        std::vector<asio::Byte> data(request.begin(), request.end());
        self->send(data);
    };
    C.on_connect_error = [&D](asio::TCPConnection *self) { clog << "Error in connecting." << endl; };
    C.on_data_received = [](asio::IStream<asio::Byte> *stream) {
        auto *self = (asio::TCPConnection*)stream;
        auto data = self->receive(256);
        clog << "Got: \n" << string(data.begin(), data.end()) << endl << endl;
    };
    C.on_output_buffer_empty = [](asio::IStream<asio::Byte> *self) {
        clog << "Sent all!" << endl;
    };
    C.connect("localhost", 1234);
    D.add_connection(std::move(C));
    D.work();
    return 0;
}
