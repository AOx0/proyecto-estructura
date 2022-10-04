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

void resolve(shared_ptr<optional<Connection>> s, TcpServer &tcp, shared_ptr<Logger> log) {
#define SEND(...) send << fmt::format(__VA_ARGS__)

    stringstream send;

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
        LOG("Creating database {}", tokens[2]);

        // Create database
        DataBase::create("data/", tokens[2]);
      }


    }
    tcp.send(s->value(),send.str());
}

int main() {
  vector<thread> threads;
  FileManager::Path data_path(FileManager::Path::get_project_dir("com", "up", "toi"));
  if(const char* env_p = std::getenv("TOI_DATA_PATH"))
    data_path = env_p;

  if (!data_path.exists()) {
    Logger::show(LOG_TYPE_::WARN, fmt::format("Data path {} does not exist", data_path.path));
    Logger::show(LOG_TYPE_::WARN, fmt::format("Creating data path.", data_path.path));
    if (!data_path.create_as_dir()) {
      Logger::show(LOG_TYPE_::ERROR, fmt::format("Failed to create file {}", data_path.path));
      return 1;
    }
  } else if (data_path.is_file()) {
    Logger::show(LOG_TYPE_::WARN, fmt::format("Data path {} exist but is a file", data_path.path));

    Logger::show(LOG_TYPE_::WARN, fmt::format("Removing file {}", data_path.path));
    // remove data path
    if (!data_path.remove()) {
      Logger::show(LOG_TYPE_::ERROR, fmt::format("Failed to remove file {}", data_path.path));
      return 1;
    }

    Logger::show(LOG_TYPE_::WARN, fmt::format("Creating data path folder.", data_path.path));
    if (!data_path.create_as_dir()) {
      Logger::show(LOG_TYPE_::ERROR, fmt::format("Failed to create folder {}", data_path.path));
      return 1;
    }
  }

  // Create logger.
  shared_ptr<Logger> log(make_shared<Logger>(data_path/"log.log"));

  LOG("Logger initialized!");

  FileManager::Path app_data (data_path/"data");

  if (!app_data.exists()) {
    WARN("App data path {} does not exist", app_data.path);
    WARN("Creating app data path.", app_data.path);
    if (!app_data.create_as_dir()) {
      ERROR("Failed to create file {}", app_data.path);
      return 1;
    }
  } else if (app_data.is_file()) {
    WARN("App data path {} does exist but is a file", app_data.path);

    WARN("Removing file {}", app_data.path);
    if (!app_data.remove()) {
      ERROR("Failed to remove file {}", app_data.path);
      return 1;
    }

    WARN("Creating app data path folder.", app_data.path);
    if (!app_data.create_as_dir()) {
      ERROR("Failed to create dir {}", app_data.path);
      return 1;
    }
  }

  LOG("Data path     : {}", data_path.path);
  LOG("App data path : {}", app_data.path);
  LOG("Logger path   : {}", log->path_.path);

  LOG("Setting working directory to {}", data_path.path);

  if (!FileManager::Path::set_working_dir(data_path)) {
    ERROR("Failed to set working directory to {}", data_path.path);
    return 1;
  }

  signal(SIGINT, [](int value){ kill_sign(); st = 1;  });
  signal(SIGTERM, [](int value){ kill_sign(); st = 1;  });


  LOG("Starting CppServer 0.1.14");
  {
    LOG("Starting TcpServer 0.1.11");
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<optional<Connection>> event = server.recv();
      if (event->has_value()) {
        threads.emplace_back(
            thread([event, &server, log] { resolve(event, server, log); }));
      }
    }
    
    WARN("Shutting down server");
  }

  for (auto &t : threads) {
    t.join();
  }
}
