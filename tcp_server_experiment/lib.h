#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>

using namespace std;

struct Shared {
    uint8_t *value;
    uint8_t typ;
    bool null;
    size_t token;
};

struct Tcp {
    uint8_t *runtime;
    uint8_t *recv_signal;
    uint8_t *channels;
};

extern "C" void communicate(Tcp &, Shared &s, char *);

extern "C" void drop_shared(Shared s);

extern "C" Shared receive(Tcp &);

extern "C" Tcp start();

extern "C" void stop(Tcp);

extern "C" void kill_sign();

class Connection {
    bool null;
    Shared shared;

public:
    explicit Connection(Shared s) : shared(s) {
        null = shared.null;
    }

    bool is_null() const {
        return null;
    }

    Shared &get_shared() {
        return shared;
    }

    ~Connection() {
        if (!is_null()) {
            drop_shared(shared);
        }
    }

    string get_msg() const {
        string result;

        if (shared.null || shared.typ != 1) {
            result = (string)NULL;
        } else {
            result = (char *)shared.value;
        }

        return result;
    }
};

class TcpServer {
protected:
    Tcp server;

public:
    TcpServer() : server(start()) {}
    ~TcpServer() {
        stop(server);
    }

    shared_ptr<Connection> recv() {
        return make_shared<Connection>(receive(server));
    }

    void send(Connection &s, const string &msg) {
        char *m = (char *)msg.c_str();
        communicate(server, s.get_shared(), m);
    }
};
