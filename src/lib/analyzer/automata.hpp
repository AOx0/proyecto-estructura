#ifndef AUTOMATA_HPP
#define AUTOMATA_HPP

#include "parser.hpp"

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
    std::map<std::string, Parser::Type> columns;
  };

  struct CreateDatabase {
    std::string name;
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
    std::map<std::string, std::variant<std::string, int, float>> values;
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
    Unknown [[maybe_unused]]
  };

  using Action = std::variant<Automata::CreateDatabase, Automata::CreateTable, Automata::DeleteDatabase, Automata::DeleteTable>;

  cpp::result<Automata::Action, std::string> get_action_struct(std::vector<Parser::Token> in, std::string original);
}

#endif //AUTOMATA_HPP
