#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#include <fmt/core.h>
#include <fmt/color.h>

#include "lib/server.hpp"
#include "lib/database.hpp"
#include "lib/fm.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

void resolve(const shared_ptr<optional<Connection>>& s, TcpServer &tcp) {
#define SEND(...) tcp.send(s->value(),fmt::format(__VA_ARGS__))

    std::optional<string> r = s->value().get_msg();
    if(r.has_value()) {

      string &query = r.value();

      if (query == "stop") { kill_sign(); st = 1; return; }

      SEND("Wait a minute, processing...!\n");

      if (query.find("CREATE DATABASE") != string::npos) {

        // Split query by spaces
        istringstream iss(query);
        vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}};

        // Get database name
        SEND("    Creating database {}\n", tokens[2]);

        // Create database
        DataBase::create("data/", tokens[2]);
      }


    }
    SEND("end");
}

int main() {
  cout << "Starting CppServer 0.1.11 ..." << endl;
  vector<thread> threads;

  FileManager::Path data_path(FileManager::Path::get_project_dir("com", "up", "toi"));
  if(const char* env_p = std::getenv("TOI_DATA_PATH"))
    data_path = env_p;

  data_path /= "data";

  if (!data_path.exists() || !data_path.is_dir()) {

    // If data_path is a file, remove it
    if (data_path.exists() && !data_path.is_dir()) {
      // remove and handle error in case it return false
      if (!data_path.remove()) {
        fmt::print(fg(fmt::color::red), "Error removing file: {}\n", data_path.path);
        return 1;
      }
    }

    if (!data_path.create_as_dir()) {
      fmt::print(fg(fmt::color::red), "Error: Cannot create data directory: {}\n", data_path.path);
      return 1;
    }
  }

  fmt::print(fg(fmt::terminal_color::yellow), "Info:");
  fmt::print(fg(fmt::terminal_color::bright_white), " Data path {}\n", data_path.path);


  signal(SIGINT, [](int value){ kill_sign(); st = 1; });
  signal(SIGTERM, [](int value){ kill_sign(); st = 1; });

  {
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<optional<Connection>> event = server.recv();
      if (event->has_value()) {
        threads.emplace_back(
            thread([event, &server] { resolve(event, server); }));
      }

      // Try to join threads and remove from vector
      for (auto it = threads.begin(); it != threads.end();) {
        if (it->joinable()) {
          fmt::print(fg(fmt::terminal_color::yellow), "Info:");
          fmt::print(fg(fmt::terminal_color::bright_white), " Joining thread\n");
          it->join();
          fmt::print(fg(fmt::terminal_color::yellow), "Info:");
          fmt::print(fg(fmt::terminal_color::bright_white), " Erasing thread\n");
          it = threads.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  cout << "Finishing CPP..." << endl;
  for (auto &t : threads) {
    t.join();
  }
}
