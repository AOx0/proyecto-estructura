#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <cstring>
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
  string recv() {
    char * rec = (char *)receive(server);
    string res(rec);
    free((void *)rec);
    return res;
  }

  void send(const string & msg) {
    const char * msg2 =  msg.c_str();
    vector<char> msg3(msg2, msg2 + strlen(msg2));
    communicate(server, (uint8_t *)&msg3[0]);
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
    if (r == "finish") break;
  }

  return 0;
}
