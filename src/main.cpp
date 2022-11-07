#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#include <fmt/core.h>
#include <fort.hpp>

#include "lib/analyzer.hpp"
#include "lib/database.hpp"
#include "lib/fm.hpp"
#include "lib/linkedList.hpp"
#include "lib/logger.hpp"
#include "lib/server.hpp"
#include "lib/columnInstance.hpp"
#include "lib/databases.hpp"


using namespace std;

volatile sig_atomic_t st = 0;

#define LOG(...) log->log(fmt::format(__VA_ARGS__))
#define WARN(...) log->warn(fmt::format(__VA_ARGS__))
#define ERROR(...) log->error(fmt::format(__VA_ARGS__))
#define KILL_MSG(msg)                                                          \
  {                                                                            \
    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Received {}", msg));         \
    Logger::show_ln(LOG_TYPE_::WARN,                                           \
                    fmt::format("Shutting down everything!"));                 \
    Logger::show_ln(LOG_TYPE_::WARN,                                           \
                    fmt::format("Sending kill sign to TcpServer"));            \
    kill_sign();                                                               \
    st = 1;                                                                    \
  }

void resolve(const shared_ptr<Connection> &s, TcpServer &tcp,
             const shared_ptr<Logger> &log, Databases &dbs) {
#define SEND(...) send << fmt::format(__VA_ARGS__)
#define SEND_ERROR(...)                                                        \
  send << Logger::show(LOG_TYPE_::ERROR, fmt::format(__VA_ARGS__))
#define LSEND(...)                                                             \
  {                                                                            \
    LOG(__VA_ARGS__);                                                          \
    SEND(__VA_ARGS__);                                                         \
  }

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

    auto parser_validation_result = Parser::validate(query);
    if (!std::get<0>(parser_validation_result)) {
      SEND_ERROR("Invalid query: {}\n", std::get<1>(parser_validation_result).value());
      break;
    }

    auto ast = Parser::parse(query);
    auto args = Automata::get_action_struct(ast, query);

    if (args.has_value()) {
      if (holds_alternative<Automata::CreateDatabase>(args.value())) {
        auto arg = get<Automata::CreateDatabase>(args.value());
        LOG("Creating database {}", arg.name);
        auto db_result = DataBase::create(arg.name);
        if (db_result.has_value()) {
          LSEND("Database {} created\n", arg.name);
          dbs.add(arg.name);
          LSEND("Databases {}\n", dbs.to_string());
        } else {
          SEND_ERROR("{}\n", db_result.error());
        }
      } else if (holds_alternative<Automata::DeleteDatabase>(args.value())) {
        auto arg = get<Automata::DeleteDatabase>(args.value());

        auto result = dbs.delete_database(arg.name);

        if (result.has_error()) {
          SEND_ERROR("{}\n", result.error());
        } else {
          LSEND("Deleted database {}\n", arg.name);
        }

      } else if (holds_alternative<Automata::DeleteTable>(args.value())) {
        auto arg = get<Automata::DeleteTable>(args.value());

        auto result = dbs.delete_table(arg.table, arg.database);

        if (result.has_error()) {
          SEND_ERROR("{}\n", result.error());
        } else {
          LSEND("Deleted table {}.{}\n", arg.database, arg.table);
        }
      } else if (holds_alternative<Automata::CreateTable>(args.value())) {
        auto arg = get<Automata::CreateTable>(args.value());
        auto db = dbs.dbs.get(arg.db);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.name);
        } else {
          auto result = DatabaseTable::createTable(arg.db, arg.name, arg.columns);
          if (result.has_error()) {
            SEND_ERROR("{}\n", result.error());
          } else {
            db->tables.insert(arg.name,
                              std::make_shared<DatabaseTable>(std::move(*result)));
            LSEND("DatabaseTable {} created in database {}\n", arg.name, arg.db);
          }
        }
      } else if (holds_alternative<Automata::ShowDatabase>(args.value())) {
        auto arg = get<Automata::ShowDatabase>(args.value());
        auto db = dbs.dbs.get(arg.name);
        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.name);
        } else {
          (*db->using_db)++;
            fort::char_table pp_table;
            pp_table << fort::header << "Tables" << fort::endr;
          db->tables.for_each_c(
              [&](const KeyValue<std::string, std::shared_ptr<DatabaseTable>> &table) {
                pp_table << table.key << fort::endr;
                return false;
              });

          send << pp_table.to_string() << '\n';
          (*db->using_db)--;
        }
      } else if (holds_alternative<Automata::ShowTable>(args.value())) {
        auto arg = get<Automata::ShowTable>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;
          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            fort::char_table pp_table;
            pp_table << fort::header << "Column" << "Size" << "Type" << fort::endr;
            auto _ = (*table)->columns.for_each_c(
                [&](const KeyValue<std::string, Layout> &column) {
                    pp_table << column.key << column.value.size << to_string(column.value.type)
                             << fort::endr;
                  return false;
                });

            send << pp_table.to_string() << '\n';
          }

          (*db->using_db)--;
        }

      } else if (holds_alternative<Automata::ShowDatabases>(args.value())) {
        auto databases = dbs.get_db_names();
        if (databases.empty()) {
          SEND_ERROR("No databases exist\n");
        } else {
          fort::char_table pp_table;
            pp_table << fort::header << "Databases" << fort::endr;
          for (auto &db : databases) {
            pp_table << db << fort::endr;
          }
          send << pp_table.to_string() << '\n';
        }
      } else if (holds_alternative<Automata::Insert>(args.value())) {
        auto arg = std::get<Automata::Insert>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            auto result =
                (*table)->try_insert(arg.database, std::move(arg.values));
            if (result.has_error()) {
              SEND_ERROR("{}\n", result.error());
            } else {
              SEND("Values inserted successfully!\n");
            }
          }

          (*db->using_db)--;
        }
      } else if (holds_alternative<Automata::ShowColumnValues>(args.value())) {
        auto arg = std::get<Automata::ShowColumnValues>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            auto result = ColumnInstance::load_column(
                arg.database, arg.table, arg.column, *(*table));
            if (result.has_error()) {
              SEND_ERROR("{}\n", result.error());
            } else {
              auto string_version = result.value().to_string_vec();

              fort::char_table pp_table;
              pp_table << fort::header << "Values" << fort::endr;
              for (auto & value : string_version) {
                pp_table << value << fort::endr;
              }

              send << pp_table.to_string() << '\n';
            }
          }

          (*db->using_db)--;
        }
      }
      else if(holds_alternative<Automata::ShowTableData>(args.value())){
        auto arg = std::get<Automata::ShowTableData>(args.value());
        auto db = dbs.dbs.get(arg.database);
        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            vector<vector<string>> data;
            vector<string> column_names;

            (*table)->columns.for_each_c([&](const KeyValue<std::string, Layout> &column){
              auto result = ColumnInstance::load_column(
                  arg.database, arg.table, column.key, *(*table));
              column_names.push_back(column.key);
              if (result.has_error()) {
                SEND_ERROR("{}\n", result.error());
              } else {
                data.emplace_back(result.value().to_string_vec());
              }
              return false;
            });

            fort::char_table pp_table;

            for (auto &column_name : column_names) {
              pp_table << fort::header << column_name;
            }
            pp_table << fort::endr;


            for (size_t i = 0; i < data[0].size(); i++) {
              for (auto & j : data) {
                pp_table << j[i];
              }
                pp_table << fort::endr;
            }

            send << pp_table.to_string() << "\n";
          }

          (*db->using_db)--;
        }
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
  Databases dbs;
  vector<thread> threads;
  FileManager::Path data_path(
      FileManager::Path::get_project_dir("com", "up", "toi"));
  if (const char *env_p = std::getenv("TOI_DATA_PATH"))
    data_path = env_p;

  if (!data_path.exists()) {
    Logger::show_ln(LOG_TYPE_::WARN,
                    fmt::format("Data path {} does not exist", data_path.path));
    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Creating data path folder"));
    if (!data_path.create_as_dir()) {
      Logger::show_ln(LOG_TYPE_::ERROR,
                      fmt::format("Failed to create file {}", data_path.path));
      return 1;
    }
  } else if (data_path.is_file()) {
    Logger::show_ln(
        LOG_TYPE_::WARN,
        fmt::format("Data path {} exist but is a file", data_path.path));

    Logger::show_ln(LOG_TYPE_::WARN,
                    fmt::format("Removing file {}", data_path.path));
    // remove data path
    if (!data_path.remove()) {
      Logger::show_ln(LOG_TYPE_::ERROR,
                      fmt::format("Failed to remove file {}", data_path.path));
      return 1;
    }

    Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Creating data path folder"));
    if (!data_path.create_as_dir()) {
      Logger::show_ln(
          LOG_TYPE_::ERROR,
          fmt::format("Failed to create folder {}", data_path.path));
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

  auto db_load_result = dbs.load();
  if (db_load_result.has_error()) {
    ERROR("{}", db_load_result.error());
    return 1;
  }

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
      threads.emplace_back(thread(
          [event, &server, log, &dbs] { resolve(event, server, log, dbs); }));
    }

    WARN("Shutting down TcpServer");
  }

  Logger::show_ln(LOG_TYPE_::WARN, fmt::format("Shutting down CppServer"));
  for (auto &t : threads) {
    t.join();
  }
}
