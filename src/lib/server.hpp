#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>

using namespace std;

struct Shared {
  uint8_t *value;
  bool null;
  size_t token;
};

extern "C" bool communicate(void *, Shared &s, const char *);

extern "C" void drop_shared(Shared s);

extern "C" Shared receive(void *);

extern "C" void * start();

extern "C" void stop(void *);

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
    if (shared.null) {
      return std::nullopt;
    } else {
      return std::make_optional(string((char *)shared.value));
    }
  }
};

class TcpServer {
protected:
  void * server;

public:
  TcpServer() : server(start()) {}
  ~TcpServer() { stop(server); }

  shared_ptr<Connection> recv() {
    return make_shared<Connection>(receive(server));
  }

  bool send(Connection &s, const string &msg) {
    auto m = msg.c_str();
    return communicate(server, s.get_shared(), m);
  }
};
