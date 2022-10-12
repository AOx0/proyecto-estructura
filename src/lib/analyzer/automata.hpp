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
    std::string name;
    std::map<std::string, Parser::Type> columns;
  };

  struct CreateDatabase {
    std::string nombre;
  };

  struct DeleteTable {
    std::string name;
  };

  struct DeleteDatabase {
    std::string name;
  };

  struct MakeSelect {
    std::map<std::string, std::set<std::string>> fields_to_select;
    std::map<std::string, Parser::Type> columns;
  };


  enum Context {
    CreateDatabaseE,
    Unknown [[maybe_unused]]
  };

  cpp::result<std::optional<std::variant<Automata::CreateDatabase>>, std::string> get_action_struct(std::vector<Parser::Token> in, std::string original);
}

#endif //AUTOMATA_HPP
