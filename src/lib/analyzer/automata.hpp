#ifndef AUTOMATA_HPP
#define AUTOMATA_HPP

#include "parser.hpp"
#include "../linkedList.hpp"
#include "../table.hpp"

#include <map>
#include <set>
#include <string>
#include <variant>
#include <result.hpp>

namespace Automata {
  template<class... Ts>
  struct overload : Ts ... {
    using Ts::operator()...;
  };
  template<class... Ts>
  overload(Ts...) -> overload<Ts...>;

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
    std::string table;
    KeyValueList<std::string, std::variant<std::string, int, float>> values;
  };

  struct MakeSelect {
    std::map<std::string, std::set<std::string>> fields_to_select;
    std::map<std::string, Parser::Type> columns;
  };


  enum Context {
    CreateDatabaseE,
    CreateTableE,
    DeleteDatabaseE,
    DeleteTableE,
    ShowDatabaseE,
    ShowTableE,
    Unknown [[maybe_unused]]
  };

  using Action = std::variant<
      Automata::CreateDatabase,
      Automata::CreateTable,
      Automata::DeleteDatabase,
      Automata::DeleteTable,
      Automata::ShowDatabase,
      Automata::ShowTable
  >;

  cpp::result<Automata::Action, std::string> get_action_struct(std::vector<Parser::Token> in, std::string original);
}

#endif //AUTOMATA_HPP