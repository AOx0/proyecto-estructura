#include <csignal>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "lib.h"

using namespace std;

volatile sig_atomic_t st = 0;

void inthand(int signum) {
  kill_sign();
  st = 1;
}

void resolve(shared_ptr<Connection> s, TcpServer &tcp) {
  if (s->is_null())
    return;
  stringstream a((string()));
  string r = s->get_msg();

  a << "Hola desde C++ " << r << "!" << endl;

  if (r == "stop")
    inthand(0);
  else
    tcp.send(*s, a.str());
}

int main() {
  cout << "Starting CppServer 0.1.11 ..." << endl;
  vector<thread> threads;

  signal(SIGINT, inthand);
  signal(SIGTERM, inthand);

  {
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<Connection> result = server.recv();
      threads.emplace_back(
          thread([result, &server] { resolve(result, server); }));
    }
  }

  cout << "Finishing CPP..." << endl;
  for (auto &t : threads) {
    t.join();
  }
}
