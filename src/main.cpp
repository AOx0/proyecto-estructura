#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

#include "lib/server.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

void resolve(shared_ptr<optional<Connection>> s, TcpServer &tcp) {
  std::optional<string> r = s->value().get_msg();

  if (r.has_value()) {
    string &query = r.value();

    // If the query is "stop", we turn down the server
    if (r == "stop") { kill_sign(); st = 1; return; }

    // Here goes the query solver
    tcp.send(s->value(), "Wait a minute, processing...\n");
  }

  // We must send the message "end" when we are done sending info
  tcp.send(s->value(), "end");
}

int main() {
  cout << "Starting CppServer 0.1.11 ..." << endl;
  vector<thread> threads;

  signal(SIGINT, [](int value){ kill_sign(); st = 1; });
  signal(SIGTERM, [](int value){ kill_sign(); st = 1; });

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
