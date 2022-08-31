#include <iostream>
#include <chrono>
#include <thread>

extern "C" void print(char* n);
extern "C" void * start_server();
extern "C" void end_server(void *);
extern "C" char* read(void *);
extern "C" void write(void *, char *);

using namespace std;

int main() {
  string in;
  //print((char *)&"Hola");

  cout << "Starting server..." << endl;
  void * server = (void *)start_server(); 
  cout << "Started!!" << endl;

  for (int i=0; i<10; i++) {
    char * received;
    received = read(server);

    write(server, "Jajaja");


  }

  cout << "Input anything to end server: ";
  cin >> in;

  end_server(server);

  return 0;
}
