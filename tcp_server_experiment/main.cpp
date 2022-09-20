#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "lib.h"

using namespace std;

bool resolve(Shared s, TcpServer & tcp) {
  stringstream a((string()));
  string r = s.get_msg();

  a << "Hola desde C++ " << r << endl;
  if (r == "exit") {
    return false;
  }

  if (r == "slow") {
    this_thread::sleep_for(chrono::seconds(10));
  }


  tcp.send(s, a.str());
  if (r == "finish")
    return true;

  return false;
}

int main() {
  string in;
  TcpServer server = TcpServer();
  vector<thread> threads;

  while (true) {
    Shared result = server.recv();
    threads.emplace_back(thread ([result, &server]{ resolve(result, server); }));
  }

  for (auto & t: threads) {
    t.join();
  }
}
