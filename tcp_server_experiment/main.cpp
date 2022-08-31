#include <iostream>
#include <chrono>
#include <thread>

extern "C" void print(char* n);
extern "C" void * start_server();
extern "C" void end_server(void *);

using namespace std;

int main() {
  string in;
  //print((char *)&"Hola");

  cout << "Starting server..." << endl;
  void * server = (void *)start_server(); 
  cout << "Started!!" << endl;
  
  cout << "Input anything to end server: ";
  cin >> in;

  end_server(server);

  return 0;
}
