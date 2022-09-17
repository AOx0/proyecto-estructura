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
};

int main() {
  string in;
  //print((char *)&"Hola");
  // Hola jajaja 
  cout << "Starting server..." << endl;
  TcpServer server = TcpServer();
  cout << "Started!!" << endl;

  cout << "Input anything to end server: ";
  cin >> in;

  return 0;
}
