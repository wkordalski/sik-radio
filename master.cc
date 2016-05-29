#include <iostream>
#include <memory>

#include "asio/driver.hh"
#include "asio/epoll.hh"
#include "asio/tcp.hh"

#include "shoutcast.hh"
#include "asio/udp.hh"

#include <boost/regex.hpp>

using namespace std;

class PlayerDriver;
class TelnetConnection;

std::map<int, std::shared_ptr<PlayerDriver>> players;
std::map<int, std::shared_ptr<TelnetConnection>> connections;

std::vector<asio::Byte> stov(std::string s) {
    return std::vector<asio::Byte>(s.begin(), s.end());
}

int player_id_gen() {
    static int player_counter = 0;
    player_counter++;
    while(players.find(player_counter) != players.end()) player_counter++;
    return player_counter;
}

int connection_id_gen() {
    static int connection_counter = 0;
    connection_counter++;
    while(connections.find(connection_counter) != connections.end()) connection_counter++;
    return connection_counter;
}

void error(std::string s, int id = -1);

class PlayerDriver {
    struct title_data {
        asio::Alarm::clock::time_point tp;
        std::function<void(std::string)> ok;
        std::function<void()> tle;
        asio::Alarm::task task;
    };
    std::shared_ptr<asio::UDPSocket> socket;
    std::deque<title_data> titles;
    asio::Slot<> message_received;
    asio::Address addr;
public:
    PlayerDriver(std::shared_ptr<asio::UDPSocket> socket, asio::Address addr)
            : socket(socket), message_received(std::bind(&PlayerDriver::on_msg_rcvd, this)), addr(addr) {
        socket->on_data_received.add(message_received);
    }

    void get_title(std::function<void(std::string)> ok, std::function<void()> tle) {
        auto tp = asio::Alarm::clock::now() + std::chrono::seconds(3);
        title_data td;
        td.tp = tp;
        td.ok = ok;
        td.tle = tle;
        titles.push_back(td);
        auto t = socket->get_driver().get_alarm()->at(tp, std::bind(&PlayerDriver::on_alarm, this));
        titles[titles.size() - 1].task = t;
        socket->send({asio::Message({'T','I','T','L','E'}, addr)});
    }
    void command(std::string cmd) {
        socket->send({asio::Message(std::vector<asio::Byte>(cmd.begin(), cmd.end()), addr)});
    }

protected:
    void on_msg_rcvd() {
        int msgs = socket->get_input_buffer_size();
        auto v = socket->receive(msgs);
        for(auto vv : v) {
            std::string title(vv.data.begin(), vv.data.end());
            this->on_title(title);
        }
    }
    void on_title(std::string data) {
        if(titles.empty()) {
            error("Got unexpected message from player.");
        } else {
            title_data td = titles.front();
            titles.pop_front();
            socket->get_driver().get_alarm()->cancel(td.task);
            td.ok(data);
        }
    }

    void on_alarm() {
        titles.pop_front();
    }
};

class TelnetConnection {
public:
    int id;
    std::shared_ptr<asio::TCPSocket> socket;
    asio::Processor<asio::TCPSocket> *processor;
    asio::Slot<std::string> do_command;
    asio::tasks::ReadTelnetLine *line_reader;
    TelnetConnection(int id, std::shared_ptr<asio::TCPSocket> socket)
            : id(id), socket(socket), do_command(std::bind(&TelnetConnection::command, this, std::placeholders::_1)) {
        processor = new asio::Processor<asio::TCPSocket>(socket);
        line_reader = new asio::tasks::ReadTelnetLine();
        line_reader->on_line_received.add(do_command);
        processor->task(line_reader);
    }

    void command(std::string cmd) {
        static boost::regex c_start("START ([A-Za-z0-9]+) (.*)");
        static boost::regex c_at(R"(AT (\d{1,2})\.(\d{1,2}) (\d+) ([A-Za-z0-9]+) (.*))");
        static boost::regex c_player(R"((PLAY|PAUSE|TITLE|QUIT) (\d+))");
        static boost::regex c_connect(R"(CONNECT ([A-Za-z0-9]+) (\d+))");

        boost::smatch M;

        if(boost::regex_match(cmd, M, c_start)) {
            // START
        } else if(boost::regex_match(cmd, M, c_at)) {
            // AT
        } else if(boost::regex_match(cmd, M, c_player)) {
            std::string cmd = M[1];
            int id = std::stoi(M[2]);
            auto plr = players.find(id);
            if(plr == players.end()) {
                // TODO: error
            } else {
                if(cmd == "TITLE") {
                    auto guard = connections[id];
                    plr->second->get_title(
                            [this,guard](std::string s){ s+= "\r\n"; this->socket->send(std::vector<asio::Byte>(s.begin(), s.end()));},
                            [this]() { /* TODO: error - TLE */ }
                    );
                } else {
                    plr->second->command(cmd);
                }
            }
            // PLAYER
        } else if(boost::regex_match(cmd, M, c_connect)) {
            // CONNECT
            auto adrs = asio::Address::get_address(M[1], M[2],
                                                   {asio::Address::Family::Inet4},
                                                   {asio::Address::SocketType::Datagram},
                                                   {asio::Address::Protocol::UDP});
            if(adrs.empty()) {
                // TODO: error
            } else {
                auto C = socket->get_driver().make_socket<asio::UDPSocket>();
                C->ready([C](){
                    std::cerr << "Bound!" << std::endl;
                    C->bind();
                });
                auto T = std::make_shared<PlayerDriver>(C, adrs[0]);
                int id = player_id_gen();
                players[id] = T;
                std::cerr << "Connected." << std::endl;
                this->socket->send(stov("OK " + std::to_string(id)));
            }
        } else {
            // TODO: error
        }
    }
};

void error(std::string s, int id) {
    for(auto c : connections) {
        if(id > 0) {
            c.second->socket->send(stov(std::string("ERROR ") + std::to_string(id) + " " + s + "\r\n"));
        } else {
            c.second->socket->send(stov(std::string("ERROR ") + s + "\r\n"));
        }
    }
}

void accepted(std::shared_ptr<asio::TCPListener<asio::TCPSocket>> L) {
    asio::Address addr;
    auto C = L->accept(addr);
    int id = connection_id_gen();
    auto T = std::make_shared<TelnetConnection>(id, C);
    connections[id] = T;
}

int main() {
    asio::Driver D(new asio::Epoll());
    auto L = D.make_socket<asio::TCPListener<asio::TCPSocket>>();
    asio::Slot<> acc(std::bind(&accepted, L));
    L->on_connection_accepted.add(acc);
    L->ready([L](){
        L->listen("localhost", 4321);
    });
    //D.get_alarm()->at(asio::Alarm::clock::now() + std::chrono::seconds(30), [&D](){D.stop();});
    D.work();
    clog << "Quited!!!" << endl;
    return 0;
}
