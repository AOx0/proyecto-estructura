#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>
#include <optional>

#include "lib/server.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

void inthand(int signum) {
  kill_sign();
  st = 1;
}

void resolve(shared_ptr<optional<Connection>> s, TcpServer &tcp) {
  stringstream a((string()));
  std::optional<string> r = s->value().get_msg();

  if (r.has_value()) {
    string & query = r.value();

    // Here goes the query handling

    // For example, if it is equal to "stop", we turn down the server
    if (r == "stop")
      inthand(0);
    else
      tcp.send(s->value(), a.str());
  } else {
    tcp.send(s->value(), "Error getting message");
  }
}

int main() {
  cout << "Starting CppServer 0.1.11 ..." << endl;
  vector<thread> threads;

  signal(SIGINT, inthand);
  signal(SIGTERM, inthand);

  {
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<optional<Connection>> event = server.recv();
      if (event->has_value()) {
        threads.emplace_back(
            thread([event, &server] { resolve(event, server); }));
      }
    }
  }

  cout << "Finishing CPP..." << endl;
  for (auto &t : threads) {
    t.join();
  }
}
