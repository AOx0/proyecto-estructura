#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#include <fmt/core.h>

#include "lib/server.hpp"
#include "lib/database.hpp"
#include "lib/fm.hpp"
#include "lib/logger.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

#define LOG(...) log->log(fmt::format(__VA_ARGS__))
#define WARN(...) log->warn(fmt::format(__VA_ARGS__))
#define ERROR(...) log->error(fmt::format(__VA_ARGS__))

void resolve(const shared_ptr<optional<Connection>>& s, TcpServer &tcp, const shared_ptr<Logger> & log) {
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
        LOG("    Creating database {}", tokens[2]);

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

  shared_ptr<Logger> log(make_shared<Logger>(data_path/"log.log"));

  data_path /= "data";

  if (!data_path.exists() || !data_path.is_dir()) {

    // If data_path is a file, remove it
    if (data_path.exists() && !data_path.is_dir()) {
      // remove and handle error in case it return false
      if (!data_path.remove()) {
        ERROR("Failed to remove file {}", data_path.path);
        return 1;
      }
    }

    if (!data_path.create_as_dir()) {
      ERROR("Cannot create data directory: {}", data_path.path);
      return 1;
    }
  }

  LOG("Data path {}", data_path.path);

  signal(SIGINT, [](int value){ kill_sign(); st = 1; });
  signal(SIGTERM, [](int value){ kill_sign(); st = 1; });

  {
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<optional<Connection>> event = server.recv();
      if (event->has_value()) {
        threads.emplace_back(
            thread([event, &server, log] { resolve(event, server, log); }));
      }

      // Try to join threads and remove from vector
      for (auto it = threads.begin(); it != threads.end();) {
        if (it->joinable()) {
          it->join();
          it = threads.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  WARN("Shutting down server");
  for (auto &t : threads) {
    t.join();
  }
}
