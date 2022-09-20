#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "lib.h"

using namespace std;

int main() {
  string in;
  
  TcpServer server = TcpServer();

  Shared result;
  stringstream a;
  while (true) {
    result = server.recv();
    result.get_msg()

    a.str(string());
    a << "Hola desde C++ " << result << endl;
    if (result == "exit") {
      continue;
    }
    server.send(a.str());
    if (result == "finish")
      break;

  }

  return 0;
}
