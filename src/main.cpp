#include <csignal>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>
#include <fmt/core.h>
#include <fort.hpp>
#include <cxxopts.hpp>

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
            (*db)->tables.insert(arg.name,
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
            // We read lock the database
            std::shared_lock<std::shared_mutex> lock((*db)->mutex);

            fort::char_table pp_table;
            pp_table << fort::header << "Tables" << fort::endr;
          (*db)->tables.for_each_c(
              [&](const KeyValue<std::string, std::shared_ptr<DatabaseTable>> &table) {
                pp_table << table.key << fort::endr;
                return false;
              });

          send << pp_table.to_string() << '\n';
        }
      } else if (holds_alternative<Automata::ShowTable>(args.value())) {
        auto arg = get<Automata::ShowTable>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          // We read lock the database
          std::shared_lock<std::shared_mutex> lock((*db)->mutex);

          auto table = (*db)->tables.get(arg.table);


          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            // We read lock the database
            std::shared_lock<std::shared_mutex> lock((*table)->mtx_);

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
          auto table = (*db)->tables.get(arg.table);
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
        }
      } else if (holds_alternative<Automata::ShowColumnValues>(args.value())) {
        auto arg = std::get<Automata::ShowColumnValues>(args.value());
        auto db = dbs.dbs.get(arg.database);

        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {
          // We read lock the database
          std::shared_lock<std::shared_mutex> lock((*db)->mutex);

          auto table = (*db)->tables.get(arg.table);

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
        }
      }
      else if(holds_alternative<Automata::ShowTableData>(args.value())){
        auto arg = std::get<Automata::ShowTableData>(args.value());
        auto db = dbs.dbs.get(arg.database);
        if (db == nullptr) {
          SEND_ERROR("Database {} does not exist\n", arg.database);
        } else {

          // We read lock the database
          std::shared_lock<std::shared_mutex> lock((*db)->mutex);

          auto table = (*db)->tables.get(arg.table);


          if (table == nullptr) {
            SEND_ERROR("DatabaseTable {} does not exist in {}\n", arg.table,
                       arg.database);
          } else {
            // We read lock the database
            // std::shared_lock<std::shared_mutex> lock((*table)->mtx_);

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

            if (data.empty()) {
              SEND_ERROR("No data in table\n");
            } else {
              for (size_t i = 0; i < data[0].size(); i++) {
                for (auto & j : data) {
                  pp_table << j[i];
                }
                pp_table << fort::endr;
              }
              send << pp_table.to_string() << "\n";
            }
          }

        }
      } else if (std::holds_alternative<Automata::Show_Select>(args.value())) {
        auto arg = std::get<Automata::Show_Select>(args.value());

        LSEND("Requested select in database {}\n", arg.database);
        LSEND("With request for displaying columns:\n");

        Cortina<std::string, std::string> display_columns;
        Cortina<std::string, Node<std::string> *> display_columns_final;

        for (auto &display_column : arg.tables) {
          LSEND("Column {}\n", display_column.sub);
          auto db = dbs.dbs.get(arg.database);

          if (db == nullptr) {
            SEND_ERROR("Database {} does not exist\n", arg.database);
          } else {

            auto table = (*db)->tables.get(display_column.name);

            if (table == nullptr) {
              SEND_ERROR("DatabaseTable {} does not exist in {}\n",
                         display_column.name, arg.database);
            } else {

              if (display_columns.g(fmt::format("{}.{}", display_column.name, display_column.sub)) == nullptr) {
                auto result = ColumnInstance::load_column(
                    arg.database, display_column.name, display_column.sub,
                    *(*table));
                if (result.has_error()) {
                  SEND_ERROR("{}\n", result.error());
                } else {
                  std::vector<std::string> string_data = result.value().to_string_vec();

                  for (auto &i : string_data)
                    display_columns.append(fmt::format("{}.{}", display_column.name, display_column.sub), i);
                }
              }
            }
          }
        }

        ListFre<size_t> indexes;
        for (size_t i = 0; i < display_columns.first()->value.value.len(); i++) {
          indexes.push(i);
        }

        // Load each of the columns that are used in the where clause
        // and then filter the data
        for (auto &where_column : arg.restrictions) {
          auto operador = std::get<1>(where_column);
          auto table_name = std::get<0>(where_column).name;
          auto column_name = std::get<0>(where_column).sub;
          auto value = std::get<2>(where_column);

          // Load column values
          auto db = dbs.dbs.get(arg.database);
          if (db == nullptr) {
            SEND_ERROR("Database {} does not exist\n", arg.database);
          } else {

            auto table = (*db)->tables.get(table_name);

            if (table == nullptr) {
              SEND_ERROR("DatabaseTable {} does not exist in {}\n", table_name,
                         arg.database);
            } else {
              auto result = ColumnInstance::load_column(
                  arg.database, table_name, column_name, *(*table));
              if (result.has_error()) {
                SEND_ERROR("{}\n", result.error());
              } else {
                for (int i = 0; i < result.value().size; i++) {
                  switch (result.value().layout.type) {
                    case u8: {
                      std::vector<uint8_t> *vector = reinterpret_cast<std::vector<uint8_t> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<uint8_t>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<uint8_t>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case u16: {
                      std::vector<uint16_t> *vector = reinterpret_cast<std::vector<uint16_t> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<uint16_t>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<uint16_t>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case u32: {
                      std::vector<uint32_t> *vector = reinterpret_cast<std::vector<uint32_t> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<uint32_t>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<uint32_t>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case u64: {
                      std::vector<uint64_t> *vector = reinterpret_cast<std::vector<uint64_t> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<uint64_t>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<uint64_t>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case f64: {
                      std::vector<double> *vector = reinterpret_cast<std::vector<double> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<double>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<double>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case str: {
                      std::vector<std::string> *vector = reinterpret_cast<std::vector<std::string> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<std::string>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<std::string>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    case rbool: {
                      std::vector<bool> *vector = reinterpret_cast<std::vector<bool> *>(result.value().data);
                      auto r = ColumnInstance::resolve_value(value, result.value().layout);
                      if (r.has_error()) {
                        SEND_ERROR("{}\n", r.error());
                      } else if (!std::holds_alternative<bool>(r.value())) {
                        SEND_ERROR("Value {} is not of type {}\n", Automata::val_to_string(value), to_string(result.value().layout.type));
                      } else {
                        auto v = std::get<bool>(r.value());
                        if (operador == Parser::OperatorE::EQUAL) {
                          if (vector->at(i) == v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT) {
                          if (vector->at(i) > v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS) {
                          if (vector->at(i) < v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::GREAT_EQUAL) {
                          if (vector->at(i) >= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::LESS_EQUAL) {
                          if (vector->at(i) <= v) {
                            indexes.push(i);
                          }
                        } else if (operador == Parser::OperatorE::NOT_EQUAL) {
                          if (vector->at(i) != v) {
                            indexes.push(i);
                          }
                        }
                      }
                    }
                    break;
                    default:
                      SEND_ERROR("Type {} not supported\n", to_string(result.value().layout.type));
                      break;
                  }
                }
              }
            }
          }
        }

        send << indexes << '\n';

        display_columns.for_each_node([&](Node<KeyValue<string, List<string>>> *columns) {
          size_t i = 0;
          columns->value.value.for_each_node([&](Node<string> *node) {
            if (indexes.get(i)->value.fre - 1 == arg.restrictions.size()) {
              auto a = columns->value.value.get(i);
              display_columns_final.append(columns->value.key,a);
            }

            i++;
            return false;
          });
          return false;
        });

        send << display_columns << '\n';
        send << display_columns_final << '\n';

        fort::char_table pp_table;
        display_columns_final.for_each_c([&](const auto &column) {
          pp_table << fort::header << column.key;
          return false;
        });
        pp_table << fort::endr;


        if (display_columns_final.len() <= 0) {
          LSEND("No results found");
        } else {
          for (int j=0; j<display_columns_final.get_at(0)->value.value.len(); j++) {
            for (int i=0; i<display_columns_final.len(); i++) {
              pp_table << display_columns_final.get_at(i)->value.value.get(j)->value->value;
            }
            pp_table << fort::endr;
          }
        }


        send << pp_table.to_string() << '\n';
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

int main(int argc, char **argv) {
  cxxopts::Options options("toidb-server", "A simple database server");
  options.add_options()("h,help", "Print help")(
      "p,port", "Port to listen on", cxxopts::value<std::string>()->default_value("9999"))(
      "i,ip", "IP to listen on", cxxopts::value<std::string>()->default_value("[::]"));

  auto args = options.parse(argc, argv);

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
    TcpServer server = TcpServer(args["ip"].as<std::string>(), args["port"].as<std::string>());

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
