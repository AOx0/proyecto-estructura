#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "lib.h"

using namespace std;

void backgroundTask() {
  cout << "Procesando..." << endl;
  this_thread::sleep_for(chrono::seconds(1));
}

int main() {
  string in;

  TcpServer server = TcpServer();
  std::vector<std::thread> threads;

  string r;
  stringstream a;
  while (true) {
    r = server.recv();

    a.str(string());
    a << "Hola desde C++ " << r << endl;

    threads.emplace_back(backgroundTask);

    if (r == "exit")
      continue;

    server.send(a.str());
    if (r == "finish")
      break;
  }

  for (thread & t: threads) t.join();

  return 0;
}
