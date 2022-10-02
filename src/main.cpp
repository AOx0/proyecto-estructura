#define SEND(...) tcp.send(s->value(),fmt::format(__VA_ARGS__));
#define PROCESS(...) std::optional<string> r = s->value().get_msg(); if(r.has_value()) { string &query = r.value(); \
if (query == "stop") { kill_sign(); st = 1; return; } __VA_ARGS__};SEND("end");

#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#include <fmt/core.h>
#include <fmt/color.h>

#include "lib/server.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

void resolve(shared_ptr<optional<Connection>> s, TcpServer &tcp) {
PROCESS(
  // We can format the response with fmt::format implicitly by calling the SEND macro
  SEND(fmt::emphasis::bold, "Wait a minute, {}!\n", "processing...");

  if (query.find("CREATE DATABASE") != string::npos) {

    // Split query by spaces
    istringstream iss(query);
    vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}};

    // Get database name
    SEND(fmt::emphasis::bold, "Creating database {}\n", tokens[2]);
  }
)
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
