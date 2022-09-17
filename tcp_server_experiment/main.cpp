#include <iostream>
#include <thread>
#include "server/target/cxxbridge/server/src/lib.rs.h"

using namespace std;

struct TcpServer {
protected:
  Tcp server;
public:
  TcpServer() : server(start()) {}
  ~TcpServer() {
    stop(server);
  }
  void send(const string & msg) {
    const char * msg2 = msg.c_str();
    communicate(server, (uint8_t *)&msg2[0]);
  }
};

int main() {
  string in;
  cout << "Starting server..." << endl;
  TcpServer server = TcpServer();
  cout << "Started!!" << endl;

  server.send("Hola desde C++\n");

  cout << "Input anything to end server: ";
  cin >> in;

  return 0;
}
