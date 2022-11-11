#ifndef AUTOMATA_HPP
#define AUTOMATA_HPP
#include "../linkedList.hpp"
#include "../table.hpp"
#include "parser.hpp"
#include <map>
#include <result.hpp>
#include <set>
#include <string>
#include <variant>

namespace Automata {
template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
template <class... Ts> overload(Ts...) -> overload<Ts...>;

struct CreateTable {
  std::string db;
  std::string name;
  KeyValueList<std::string, Layout> columns;
};

struct CreateDatabase {
  std::string name;
};

struct ShowDatabase {
  std::string name;
};

struct ShowTable {
  std::string database;
  std::string table;
};

struct DeleteTable {
  std::string database;
  std::string table;
};

struct DeleteDatabase {
  std::string name;
};

struct Insert {
  std::string database;
  std::string table;
  List<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double,
                    Parser::Bool>>
      values;
};
struct Show_Select{
  std::string database;
  std::vector<Parser::NameAndSub> tables;
  std::vector<std::tuple<Parser::NameAndSub, Parser::OperatorE, std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double,
      Parser::Bool >>>restrictions;
};


struct ShowDatabases {};

struct ShowColumnValues {
  std::string database;
  std::string table;
  std::string column;
};
struct ShowTableData{
  std::string database;
  std::string table;
};

enum Context {
  CreateDatabaseE,
  CreateTableE,
  WhereE,
  DeleteDatabaseE,
  DeleteTableE,
  ShowDatabaseE,
  ShowTableE,
  ShowDatabasesE,
  InsertE,
  SelectE,
  ShowColumnValuesE,
  ShowTableDataE,
  Unknown [[maybe_unused]]
};

using Action = std::variant<
    Automata::CreateDatabase, Automata::CreateTable, Automata::DeleteDatabase,
    Automata::DeleteTable, Automata::ShowDatabase, Automata::ShowTable,
    Automata::ShowDatabases, Automata::Insert, Automata::ShowColumnValues, Automata::ShowTableData, Automata::Show_Select>;

cpp::result<Automata::Action, std::string>
get_action_struct(std::vector<Parser::Token> in, std::string original);
} // namespace Automata

#endif // AUTOMATA_HPP
