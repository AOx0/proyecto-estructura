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

    string r, b;
    stringstream a;
    while (true) {
        r = server.recv();
        a.str(string());
        sleep(5);
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
