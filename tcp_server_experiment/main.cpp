#include "server/target/cxxbridge/rust/cxx.h"
#include "server/target/cxxbridge/server/src/lib.rs.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

using namespace std;

struct TcpServer {
protected:
  Tcp server;

public:
  TcpServer() : server(start()) {}
  ~TcpServer() { stop(server); }
  string recv() {
    rust::String rec = receive(server);
    string res(rec);
    return res;
  }

  void send(string msg) {
    rust::String msg2(msg);
    communicate(server, msg2);
  }
};

int main() {
  string in;

  cout << "Starting server..." << endl;
  TcpServer server = TcpServer();
  cout << "Started!!" << endl;

  string r, b;
  stringstream a;
  while (true) {
    r = server.recv();
    a.str(string());
    a << "Hola desde C++ " << r << endl;
    b = a.str();
    if (r == "exit") {
      continue;
    }
    server.send(b);
    cout << "Received \"" << r << "\"" << endl;
    if (r == "finish")
      break;
  }

  return 0;
}
