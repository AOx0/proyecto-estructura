#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "lib.h"

using namespace std;

#include <unistd.h>
#include <stdio.h>
#include <signal.h>

volatile sig_atomic_t st = 0;

void inthand(int signum) {
  st = 1;
}

void resolve(Shared s, TcpServer & tcp) {
  stringstream a((string()));
  string r = s.get_msg();

  a << "Hola desde C++ " << r << endl;

  if (r == "slow")
    this_thread::sleep_for(chrono::seconds(10));

  tcp.send(s, a.str());
}

int main() {
  vector<thread> threads;

  signal(SIGINT, inthand);
  signal(SIGTERM, inthand);

  {
    TcpServer server = TcpServer();

    while (!st) {
      cout << "Antes: " << st << endl;
      Shared result = server.recv();
      cout << "Despues: " << st << endl;
      threads.emplace_back(
          thread([result, &server] { resolve(result, server); }));
      cout << "Final: " << st << endl;
    }
  }


  cout << "Finishing" << endl;
  for (auto & t: threads) {
    t.join();
  }
}
