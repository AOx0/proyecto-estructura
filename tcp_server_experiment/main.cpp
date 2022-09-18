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
    cout << "    Re..." << endl;
    char * rec = (char *)receive(server);
    string res(rec);
    free((void *)rec);
    cout << "    Returning" << endl;
    return res;
  }

  void send(const string & msg) {

    cout << "     Sigue bien" << endl;
    const char * msg2 =  msg.c_str();
    vector<char> msg3(msg2, msg2 + strlen(msg2));

    cout << "     Sigue bien 2" << endl;
    communicate(server, (uint8_t *)&msg3[0]);
    cout << "     Sigue bien 3" << endl;
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
    cout << "Waiting..." << endl;
    r = server.recv();
    cout << "Sigue bien" << endl;
    a.str(string());
    a << "Hola desde C++ " << r << endl;
    cout << "Sigue bien 2" << endl;
    b = a.str();
    cout << "Sigue bien 3" << endl;
    if (r == "exit") {
      continue;
    }
    server.send(b);
    cout << "A" << endl;
    cout << "Received \"" << r << "\"" << endl;
    if (r == "finish") break;
  }

  return 0;
}
