#include <iostream>
#include <memory>

#include "asio/driver.hh"
#include "asio/epoll.hh"
#include "asio/tcp.hh"

#include "shoutcast.hh"

using namespace std;
// http://radioluz.pwr.wroc.pl/luz.pls
// http://stream3.polskieradio.pl:8906/listen.pls
// http://www.radio.kielce.pl/themes/basic/radio_1/player/radio-kielce.pls
// http://panel.nadaje.com:9212/radiokatowice.m3u
int main() {
    asio::Driver D(new asio::Epoll());
    auto C = D.make_connection<asio::TCPConnection>();
    auto alarm = D.get_alarm();
    alarm->at(asio::Alarm::clock::now() + std::chrono::seconds(120), [&D, C](){C->disconnect(true);});
    ShoutCastClient shc(C, "/", {
            {"User-Agent", "Orban/Coding Technologies AAC/aacPlus Plugin 1.0 (os=Windows NT 5.1 Service Pack 2)"},
            {"Accept", "*/*"},
            {"Icy-MetaData", "1"},
            {"Connection", "close"}
    });
    C->ready([C](){
        C->connect("198.100.125.242", 80);
    });
    D.work();
    clog << "Quited!!!" << endl;
    return 0;
}
