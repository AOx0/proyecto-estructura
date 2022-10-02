#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>

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
  explicit Connection(Shared s) : shared(s) { null = shared.null; }

  ~Connection() {
    if (!is_null()) {
      drop_shared(shared);
    }
  }

  Shared &get_shared() { return shared; }

  [[nodiscard]] bool is_null() const { return null; }

  [[nodiscard]] std::optional<string> get_msg() const {
    if (shared.null || shared.typ != 1) {
      return std::nullopt;
    } else {
      return std::make_optional(string((char *)shared.value));
    }
  }
};

class TcpServer {
protected:
  Tcp server;

public:
  TcpServer() : server(start()) {}
  ~TcpServer() { stop(server); }

  shared_ptr<optional<Connection>> recv() {
    return make_shared<optional<Connection>>(make_optional(receive(server)));
  }

  void send(Connection &s, const string &msg) {
    char *m = (char *)msg.c_str();
    communicate(server, s.get_shared(), m);
  }

  void send(Connection &s, stringstream &msg) {
    string temp(msg.str());
    char *m = (char *)temp.c_str();
    msg.str(std::string());
    communicate(server, s.get_shared(), m);
  }
};
