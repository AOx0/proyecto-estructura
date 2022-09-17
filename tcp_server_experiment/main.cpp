#include <iostream>
#include <chrono>
#include <thread>
#include "server/target/cxxbridge/rust/cxx.h"
#include "server/target/cxxbridge/server/src/lib.rs.h"

using namespace std;

int main() {
  string in;
  //print((char *)&"Hola");
  // Hola jajaja 
  cout << "Starting server..." << endl;
  Tcp server = start();
  cout << "Started!!" << endl;

  cout << "Input anything to end server: ";
  cin >> in;

  stop(server);

  return 0;
}
