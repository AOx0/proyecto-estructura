#include <iostream>
#include <cstdint>

using namespace std;

struct Shared {
    uint8_t * value;
    uint8_t typ;
    bool null;
};

struct Tcp {
    uint8_t * runtime;
    uint8_t * continue_signal;
    uint8_t * recv_signal;
    uint8_t * stay_alive;
};

extern "C" void drop_shared(Shared);

extern "C" void communicate(Tcp &, char *);

extern "C" Shared receive(Tcp &);

extern "C" Tcp start();

extern "C" void stop(Tcp);


struct TcpServer {
protected:
    Tcp server;

public:
    TcpServer() : server(start()) {}
    ~TcpServer() {
        stop(server);
    }

    string recv() {
        string result;
        Shared rec = receive(server);

        if (rec.null == true) {
            result = (string)NULL;
        } else if (rec.typ != 1) {
            result = (string)NULL;
        } else {
            string value = (char *)rec.value;
            result = value;
        }

        drop_shared(rec);
        return result;
    }

    void send(string msg) {
        char * m = (char *)msg.c_str();
        communicate(server, m);
    }
};
