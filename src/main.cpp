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
#include "lib/analyzer.hpp"

using namespace std;

volatile sig_atomic_t st = 0;

#define LOG(...) log->log(fmt::format(__VA_ARGS__))
#define WARN(...) log->warn(fmt::format(__VA_ARGS__))
#define ERROR(...) log->error(fmt::format(__VA_ARGS__))
#define KILL_MSG(msg) { \
  Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Received {}", msg)); \
  Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Shutting down everything!")); \
  Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Sending kill sign to TcpServer")); \
  kill_sign(); \
  st = 1; \
}

void resolve(const shared_ptr<Connection> &s, TcpServer &tcp, const shared_ptr<Logger> &log) {
#define SEND(...) send << fmt::format(__VA_ARGS__)
#define SEND_ERROR(...)  send << Logger::show(LOG_TYPE_::ERROR, fmt::format(__VA_ARGS__))

  stringstream send;

  std::optional<string> r = s->get_msg();
  while (r.has_value()) {
    string query(r.value());

    LOG("Received query: \"{}\"", query);

    Parser::make_tokens_explicit(query);

    LOG("Query: \"{}\"", query);

    // remove trailing spaces from start and end
    query.erase(0, query.find_first_not_of(' '));
    query.erase(query.find_last_not_of(' ') + 1);

    if (query == "stop") {
      KILL_MSG("'stop' message");
      return;
    }

    auto result = Parser::validate(query);
    if (!std::get<0>(result)) {
      SEND_ERROR("Invalid query: {}\n", std::get<1>(result).value());
      break;
    }

    auto ast = Parser::parse(query);
    auto args = Automata::get_action_struct(ast, query);

    if (args.has_value()) {
      if (holds_alternative<Automata::CreateDatabase>(args.value())) {
        auto arg = get<Automata::CreateDatabase>(args.value());
        LOG("Creating database {}", arg.name);
        auto db_result = DataBase::create("data/", arg.name);
        if (db_result.has_value()) {
          SEND("Database {} created\n", arg.name);
        } else {
          SEND_ERROR("{}\n", db_result.error());
        }
      } else if (holds_alternative<Automata::DeleteDatabase>(args.value())) {
        auto arg = get<Automata::DeleteDatabase>(args.value());
        LOG("Deleting database {}", arg.name);
        SEND("Database {} deleted\n", arg.name);
      } else if (holds_alternative<Automata::DeleteTable>(args.value())) {
        auto arg = get<Automata::DeleteTable>(args.value());
        LOG("Deleting table {} from database {}", arg.table, arg.database);
        SEND("Deleting table {} from database {}\n", arg.table, arg.database);
      } else if (holds_alternative<Automata::CreateTable>(args.value())) {
        auto arg = get<Automata::CreateTable>(args.value());
        LOG("Creating table {} in database {}", arg.name, arg.db);
        SEND("Creating table {} in database {}\n", arg.name, arg.db);
      }
    } else {
      SEND_ERROR("{}\n", args.error());
    }
    break;
  }

  if (!tcp.send(*s, send.str())) {
    ERROR("Failed to send response: \"{}\"", send.str());
  } else {
    LOG("Sent response: \"{}\"", send.str());
  }
}

int main() {
  vector<thread> threads;
  FileManager::Path data_path(FileManager::Path::get_project_dir("com", "up", "toi"));
  if (const char *env_p = std::getenv("TOI_DATA_PATH"))
    data_path = env_p;

  if (!data_path.exists()) {
    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Data path {} does not exist", data_path.path));
    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Creating data path folder"));
    if (!data_path.create_as_dir()) {
      Logger::show_ln(LOG_TYPE_::ERROR, fmt::format("Failed to create file {}", data_path.path));
      return 1;
    }
  } else if (data_path.is_file()) {
    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Data path {} exist but is a file", data_path.path));

    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Removing file {}", data_path.path));
    // remove data path
    if (!data_path.remove()) {
      Logger::show_ln(LOG_TYPE_::ERROR, fmt::format("Failed to remove file {}", data_path.path));
      return 1;
    }

    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Creating data path folder"));
    if (!data_path.create_as_dir()) {
      Logger::show_ln(LOG_TYPE_::ERROR, fmt::format("Failed to create folder {}", data_path.path));
      return 1;
    }
  }

  // Create logger.
  shared_ptr<Logger> log(make_shared<Logger>(data_path / "log.log"));

  LOG("Logger initialized!");

  FileManager::Path app_data(data_path / "data");

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

  signal(SIGINT, [](int value) {
    cout << endl;
    KILL_MSG("SIGINT");
  });

  signal(SIGTERM, [](int value) {
    cout << endl;
    KILL_MSG("SIGTERM");
  });


  LOG("Starting CppServer 0.1.14");
  {
    LOG("Starting TcpServer 0.1.11");
    TcpServer server = TcpServer();

    while (!st) {
      shared_ptr<Connection> event = server.recv();
      threads.emplace_back(
          thread([event, &server, log] { resolve(event, server, log); }));
    }

    WARN("Shutting down TcpServer");
  }

  Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Shutting down CppServer"));
  for (auto &t: threads) {
    t.join();
  }
}
