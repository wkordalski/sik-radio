#include <iostream>
#include <memory>

#include "asio/driver.hh"
#include "asio/epoll.hh"
#include "asio/tcp.hh"

#include "shoutcast.hh"
#include "asio/udp.hh"

using namespace std;

void on_recv_msg(std::shared_ptr<asio::UDPSocket> socket, ShoutCastClient *client, asio::Driver &D) {
    auto v = socket->receive(socket->get_input_buffer_size());
    auto check = [](std::vector<asio::Byte> data, std::string pattern) {
        if(data.size() != pattern.size()) return false;
        std::vector<asio::Byte> rhs(pattern.begin(), pattern.end());
        return data == rhs;
    };
    for(auto m : v) {
        if(check(m.data, "PLAY")) {
            client->play();
        } else if(check(m.data, "PAUSE")) {
            client->pause();
        } else if(check(m.data, "TITLE")) {
            asio::Message mm;
            std::string title = client->title();
            mm.data = std::vector<asio::Byte>(title.begin(), title.end());
            mm.address = m.address;
            socket->send({mm});
        } else if(check(m.data, "QUIT")) {
            D.stop();
        }
    }
}
// http://radioluz.pwr.wroc.pl/luz.pls
// http://stream3.polskieradio.pl:8906/listen.pls
// http://www.radio.kielce.pl/themes/basic/radio_1/player/radio-kielce.pls
// http://panel.nadaje.com:9212/radiokatowice.m3u
int main() {
    asio::Driver D(new asio::Epoll());
    auto C = D.make_connection<asio::TCPSocket>();
    auto U = D.make_connection<asio::UDPSocket>();

    auto alarm = D.get_alarm();
    ShoutCastClient shc(C, "/", {
            {"User-Agent", "Orban/Coding Technologies AAC/aacPlus Plugin 1.0 (os=Windows NT 5.1 Service Pack 2)"},
            {"Accept", "*/*"},
            {"Icy-MetaData", "1"},
            {"Connection", "close"}
    });

    asio::Slot<> rcv_msg(std::bind(&on_recv_msg, U, &shc, std::ref(D)));
    U->on_data_received.add(rcv_msg);

    C->ready([C](){
        C->connect("198.100.125.242", 80);
    });
    U->ready([U](){
        U->bind("0.0.0.0", 1234);
    });
    D.work();
    clog << "Quited!!!" << endl;
    return 0;
}
