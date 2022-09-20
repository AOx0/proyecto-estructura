#include <cstdint>
#include <iostream>

using namespace std;

struct Shared {
  uint8_t *value;
  uint8_t typ;
  bool null;
  size_t token;

  string get_msg() const {
    string result;

    if (null || typ != 1) {
      result = (string)NULL;
    } else {
      result = (char *)value;
    }

    return result;
  }
};

struct Tcp {
  uint8_t *runtime;
  uint8_t *recv_signal;
  uint8_t *stay_alive;
  uint8_t *channels;
};

extern "C" void communicate(Tcp &, Shared s, char *);

extern "C" Shared receive(Tcp &);

extern "C" Tcp start();

extern "C" void stop(Tcp);

struct TcpServer {
protected:
  Tcp server;

public:
  TcpServer() : server(start()) {}
  ~TcpServer() { stop(server); }

  Shared recv() {
    return receive(server);
  }

  void send(Shared s, const string &msg) {
    char *m = (char *)msg.c_str();
    communicate(server, s, m);
  }
};
