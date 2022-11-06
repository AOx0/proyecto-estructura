#include <csignal>
#include <fmt/core.h>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

#include "lib/analyzer.hpp"
#include "lib/database.hpp"
#include "lib/fm.hpp"
#include "lib/linkedList.hpp"
#include "lib/logger.hpp"
#include "lib/server.hpp"

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

struct Databases {
  std::mutex mutex;
  KeyValueList<std::string, DataBase> dbs;

  cpp::result<void, std::string> load() {
    auto current_path = FileManager::Path::get_working_dir();
    auto data_path = current_path / "data";

    if (!data_path.exists()) {
      return cpp::fail(
          fmt::format("Data folder does not exist at {}", data_path.path));
    }

    auto contents = FileManager::list_dir("data/");

    if (contents.has_error())
      return cpp::fail(contents.error());

    for (auto &entry : contents.value()) {
      if (entry.is_dir()) {
        auto db = DataBase(entry.get_file_name());
        db.load_tables();
        dbs.insert(entry.get_file_name(), db);
      }
    }

    return {};
  }

  void add(const string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    dbs.insert(name, DataBase({}));
  }

  void remove(const string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    dbs.delete_key(name);
  }

  // To string
  std::string to_string() {
    std::lock_guard<std::mutex> lock(mutex);
    std::stringstream ss;
    ss << "Databases: " << dbs << "\n";
    return ss.str();
  }
  std::vector<std::string> get_db_names() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::string> names;
    dbs.for_each([&](const KeyValue<std::string, DataBase> &keyValue) {
      names.push_back(keyValue.key);
      return false;
    });
    return names;
  }

  cpp::result<void, std::string> delete_table(std::string table,
                                              std::string name) {
    std::lock_guard<std::mutex> lock(mutex);
    auto db =
        dbs.get(name); // método get me da el apuntador de lo que estas buscando
    if (db != nullptr) {

      auto table_ptr = db->tables.get(table);

      if (table_ptr == nullptr) {
        return cpp::fail(
            fmt::format("Table {}.{} does not exist", name, table));
      } else {
        auto result = db->delete_table_dir(name, table);

        if (result.has_error())
          return cpp::fail(result.error());

        db->tables.delete_key(table);
      }
    } else {
      return cpp::fail(fmt::format("Database {} does not exist.", name));
    }

    return {};
  }

  cpp::result<void, std::string> delete_database(std::string name) {
    std::lock_guard<std::mutex> lock(mutex);

    auto result = dbs.get(name);

    if (result == nullptr) {
      return cpp::fail(fmt::format("Database {} does not exist.", name));
    } else {
      auto db_path = FileManager::Path("data") / name;
      if (!db_path.remove()) {
        return cpp::fail(fmt::format(
            "Something went wrong while deleting folder {}", db_path.path));
      }
      dbs.delete_key(name);
    }

    return {};
  }
};

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
          auto result = Table::createTable(arg.db, arg.name, arg.columns);
          if (result.has_error()) {
            SEND_ERROR("{}\n", result.error());
          } else {
            db->tables.insert(arg.name,
                              std::make_shared<Table>(std::move(*result)));
            LSEND("Table {} created in database {}\n", arg.name, arg.db);
          }
        }
      } else if (holds_alternative<Automata::ShowDatabase>(args.value())) {
        auto arg = get<Automata::ShowDatabase>(args.value());
        auto db = dbs.dbs.get(arg.name);
        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.name);
        } else {
          SEND("Database: {}\n", arg.name);
          (*db->using_db)++;
          db->tables.for_each_c(
              [&](const KeyValue<std::string, std::shared_ptr<Table>> &table) {
                stringstream data;
                data << (*table.value.get());
                SEND("{}\n", data.str());
                return false;
              });
          (*db->using_db)--;
        }
      } else if (holds_alternative<Automata::ShowTable>(args.value())) {
        auto arg = get<Automata::ShowTable>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          SEND("Database: {}\n", arg.database);
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("Table {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            stringstream data;
            data << (*table->get());
            SEND("{}\n", data.str());
          }

          (*db->using_db)--;
        }

      } else if (holds_alternative<Automata::ShowDatabases>(args.value())) {
        auto databases = dbs.get_db_names();
        if (databases.size() == 0) {
          SEND_ERROR("No databases exist\n");
        } else {
          for (int i = 0; i < databases.size(); ++i) {
            SEND("{}\n", databases[i]);
          }
        }
      } else if (holds_alternative<Automata::Insert>(args.value())) {
        auto arg = std::get<Automata::Insert>(args.value());
        LSEND("Inserting into table {} from database {} values \n", arg.table,
              arg.database);
        arg.values.for_each([&](auto value) {
          if (holds_alternative<Parser::String>(value)) {
            Parser::String val = get<Parser::String>(value);
            LSEND("{}, ", val.value);
          } else if (holds_alternative<Parser::UInt>(value)) {
            Parser::UInt val = get<Parser::UInt>(value);
            LSEND("{}, ", val.value);
          } else if (holds_alternative<Parser::Int>(value)) {
            Parser::Int val = get<Parser::Int>(value);
            LSEND("{}, ", val.value);
          } else if (holds_alternative<Parser::Double>(value)) {
            Parser::Double val = get<Parser::Double>(value);
            LSEND("{}, ", val.value);
          }
          return false;
        });
        LSEND("\n");

        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("Table {} does not exist in {}\n", arg.table,
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
        LSEND("Request to show contents of column {} from table {} at database "
              "{}\n",
              arg.column, arg.table, arg.database);

        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          (*db->using_db)++;

          auto table = db->tables.get(arg.table);

          if (table == nullptr) {
            SEND_ERROR("Table {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {

            auto result = ColumnInstance::load_column(
                arg.database, arg.table, arg.column, *table->get());
            if (result.has_error()) {
              SEND_ERROR("{}\n", result.error());
            } else {
              LSEND("Read values successfully!\n");
            }
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

  /*signal(SIGINT, [](int value) {
    cout << endl;
    KILL_MSG("SIGINT");
  });*/

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
