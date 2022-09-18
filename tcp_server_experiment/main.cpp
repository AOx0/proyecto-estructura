#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <vector>

#include "lib.h"

using namespace std;

int main() {
  string in;

  cout << "Starting server..." << endl;
  TcpServer server = TcpServer();
  cout << "Started!!" << endl;

  string r;
  stringstream a;
  while (true) {
    r = server.recv();
    a.str(string());
    a << "Hola desde C++ " << r << endl;
    if (r == "exit") {
      continue;
    }
    server.send(a.str());
    cout << "Received \"" << r << "\"" << endl;
    if (r == "finish")
      break;
  }

  return 0;
}
