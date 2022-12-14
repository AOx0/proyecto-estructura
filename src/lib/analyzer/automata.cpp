#include "automata.hpp"

#include "../database.hpp"
#include "../logger.hpp"
#include "parser.hpp"

cpp::result<Automata::Action, std::string>
Automata::get_action_struct(std::vector<Parser::Token> in,
                            std::string original) {
  using namespace Parser;
  Context ctx = Context::Unknown;
  std::optional<Action> variant = std::nullopt;
  long int token_number;

  std::optional<Token> nextp1 = std::nullopt;
  std::optional<Token> next = std::nullopt;
  std::optional<Token> curr = std::nullopt;
  std::optional<Token> prev = std::nullopt;
  std::optional<Token> prevm1 = std::nullopt;

  auto visitor = overload{
      [&](const Bool &rbool) -> cpp::result<void, std::string> {
        if (ctx == Context::InsertE) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `)` or `,` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                     !same_variant_and_value(*next,
                                             Token{Symbol{SymbolE::COMA}})) {
            return cpp::fail(fmt::format(
                "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Insert>(variant.value());
          var.values.push_back({rbool});
        } else if (ctx == Context::WhereE) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `;`, `AND` or `OR` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
              *next, Token{Symbol{SymbolE::SEMICOLON}}) &&
                     !same_variant_and_value(*next,
                                             Token{Operator{OperatorE::AND}}) &&
                     !same_variant_and_value(*next,
                                             Token{Operator{OperatorE::OR}})) {
            return cpp::fail(fmt::format(
                "Expected `;`, `AND` or `OR`  but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Show_Select>(variant.value());
          auto operador = std::get<Operator>(*prev);
          auto name_sub = std::get<Parser::NameAndSub>(std::get<Parser::Identifier>(*prevm1));
          var.restrictions.emplace_back(name_sub, operador.variant, rbool);
        }

        return {};
      },
      [&](const Keyword &keyword) -> cpp::result<void, std::string> {
        switch (keyword.variant) {

        case KeywordE::CREATE: {
          if (token_number != 0) {
            return cpp::fail(fmt::format(
                "Found `CREATE` keyword not as the query root.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }

          if (!next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE` or `TABLE` after `CREATE` but got "
                "nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                     !same_variant_and_value(next.value(),
                                             Token{Keyword{KeywordE::TABLE}})) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE` or `TABLE` after `CREATE` but got "
                "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
          }

          if (ctx == Context::Unknown) {
            if (same_variant_and_value(next.value(),
                                       Token{Keyword{KeywordE::DATABASE}})) {
              ctx = Context::CreateDatabaseE;
            } else {
              ctx = Context::CreateTableE;
            }
          }
        } break;

        case KeywordE::USING:{
          if (token_number != 0){
            return cpp::fail(fmt::format(
                "Found `Select` keyword not as the query root.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }
          if (!next.has_value()){
            return cpp::fail(fmt::format(
                "Expected `database` name but got nothing.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }
          if (ctx == Context::Unknown) {
            if (same_variant(next.value(),Token{Identifier{Name{}}})){
              auto database = std::get<Parser::Name>(std::get<Parser::Identifier>(*next));
              variant = {Automata::Show_Select{database.value,{},{}}};
              ctx = Context::SelectE;
            } else {
              ctx = Context::Unknown;
            }
          }
        }break;

        case KeywordE::SELECT:{
          if (ctx == Context::SelectE){
            if (token_number == 0){
              return cpp::fail(fmt::format(
                  "Found `Select` keyword not as the query root.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }
            else if (!next.has_value()){
              return cpp::fail(fmt::format(
                  "Expected `table.row` table.row but got nothing.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }else if (!same_variant(next.value(),Token{Identifier{NameAndSub{}}})){
              return cpp::fail(fmt::format(
                  "Expected `table.row` table.row but got nothing.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }
            else{
              ctx = Context::SelectE;
            }
          }
        }
        break;

        case KeywordE::WHERE: {
          if (ctx == Context::WhereE){
            if(!same_variant(next.value(),Token{Identifier{NameAndSub{}}})){
              return cpp::fail(fmt::format(
                  "Expected `table.row` table.row but got nothing.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }
            if (!next.has_value())
            {
              return cpp::fail(fmt::format(
                  "Expected `== or > or >= or <= 0r > or < or !=` Operator but got nothing.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));

            }


          }

        }
        break;

        case KeywordE::DELETE: {
          if (token_number != 0) {
            return cpp::fail(fmt::format(
                "Found `DELETE` keyword not as the query root.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }

          if (!next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE` or `TABLE` after `DELETE` but got "
                "nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                     !same_variant_and_value(next.value(),
                                             Token{Keyword{KeywordE::TABLE}})) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE` or `TABLE` after `DELETE` but got "
                "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
          }

          if (ctx == Context::Unknown) {
            if (same_variant_and_value(next.value(),
                                       Token{Keyword{KeywordE::DATABASE}})) {
              ctx = Context::DeleteDatabaseE;
            } else {
              ctx = Context::DeleteTableE;
            }
          }
        } break;

        case KeywordE::SHOW: {
          if (token_number != 0) {
            return cpp::fail(fmt::format(
                "Found `SHOW` keyword not as the query root.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }

          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `DATABASE`, `DATABASES`, `FROM` or "
                            "`TABLE` after `SHOW` but got nothing.\nAfter "
                            "token {} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                     !same_variant_and_value(next.value(),
                                             Token{Keyword{KeywordE::TABLE}}) &&
                     !same_variant_and_value(next.value(),
                                             Token{Keyword{KeywordE::FROM}}) &&
                     !same_variant_and_value(
                         next.value(), Token{Keyword{KeywordE::DATABASES}})) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE`, `DATABASES`, `FROM` or `TABLE` after "
                "`SHOW` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    "
                "\"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
          }

          if (ctx == Context::Unknown) {
            if (same_variant_and_value(next.value(),
                                       Token{Keyword{KeywordE::DATABASE}})) {
              ctx = Context::ShowDatabaseE;
            } else if (same_variant_and_value(
                           next.value(), Token{Keyword{KeywordE::DATABASES}})) {
              ctx = Context::ShowDatabasesE;
            } else if (same_variant_and_value(
                           next.value(), Token{Keyword{KeywordE::TABLE}})) {
              ctx = Context::ShowTableE;
            }
          }
        } break;

        case KeywordE::FROM: {
          std::cout<<"entro en el from"<<std::endl;
          if (ctx == Context::Unknown){
            if (token_number == 0) {
              return cpp::fail(fmt::format(
                  "Found `FROM` keyword not as the query root.\nAfter token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `Database name` identifier after `FROM` but got "
                              "something incorrect.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                              to_string(*curr), token_number, original));
            }
            else if (!same_variant(*next, Token{Identifier{NameAndSub{}}}) && !same_variant(*next, Token{Identifier{Name{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE.TABLE` or `DATABASE` identifier after `FROM` but got "
                  "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, to_string(*curr), original));
            }

            if (same_variant(*next, Token{Identifier{NameAndSub{}}})) {
              if (!nextp1.has_value()) {
                return cpp::fail(fmt::format(
                    "Expected `;` after `FROM DATABASE.TABLE` but got nothing.\nAfter "
                    "token {} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*next), token_number + 1, original));
              } else if (!same_variant(*nextp1,
                                       Token{Symbol{SymbolE::SEMICOLON}})) {
                return cpp::fail(fmt::format(
                    "Expected `;` after `FROM DATABASE.TABLE` but got `{}`.\nAfter "
                    "token {} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*nextp1), token_number + 1, to_string(*next),
                    original));
              }
              auto table_column = std::get<NameAndSub>(std::get<Parser::Identifier>(*next));
              variant = {Automata::ShowTableData{table_column.name, table_column.sub}};
            } else if (same_variant(*next, Token{Identifier{Name{}}})) {
              if (!nextp1.has_value()) {
                return cpp::fail(fmt::format(
                    "Expected `TABLE.COLUMN` after `FROM DATABASE` but got nothing.\nAfter "
                    "token {} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*next), token_number + 1, original));
              } else if (!same_variant(*nextp1,
                                       Token{Identifier{NameAndSub{}}})) {
                return cpp::fail(fmt::format(
                    "Expected `TABLE.COLUMN` after `FROM DATABASE` but got `{}`.\nAfter "
                    "token {} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*nextp1), token_number + 1, to_string(*next),
                    original));
              }
              auto database = std::get<Name>(std::get<Parser::Identifier>(*next)).value;
              auto table_column = std::get<NameAndSub>(std::get<Parser::Identifier>(*nextp1));
              variant = {Automata::ShowColumnValues{database, table_column.name, table_column.sub}};
              ctx = Context::ShowColumnValuesE;
            }
          }

        }break;


        case KeywordE::COLUMN: {
          if (ctx == Context::ShowColumnValuesE) {
            std::cout << "Detected column\n";
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `TABLE.COLUMN` identifier after `COLUMN` but got "
                  "nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            } else if (!same_variant(*next, Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `TABLE.COLUMN` identifier after `COLUMN` but got "
                  "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, to_string(*curr), original));
            }

            if (!nextp1.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `;` after `TABLE.COLUMN` but got nothing.\nAfter "
                  "token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number + 1, original));
            } else if (!same_variant(*nextp1,
                                     Token{Symbol{SymbolE::SEMICOLON}})) {
              return cpp::fail(fmt::format(
                  "Expected `;` after `TABLE.COLUMN` but got `{}`.\nAfter "
                  "token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*nextp1), token_number + 1, to_string(*next),
                  original));
            }
            auto &var = std::get<Automata::ShowColumnValues>(variant.value());
            auto table_column =
                std::get<NameAndSub>(std::get<Parser::Identifier>(*next));
            var.table = table_column.name;
            var.column = table_column.sub;
          }
        } break;

        case KeywordE::INTO: {
          if (!next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE.TABLE` identifier after `INTO` but got "
                "nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          } else if (!same_variant(*next, Token{Identifier{NameAndSub{}}})) {
            return cpp::fail(fmt::format(
                "Expected `DATABASE.TABLE` identifier after `INTO` but got "
                "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
          }
        } break;
        case KeywordE::INSERT: {
          if (token_number != 0) {
            return cpp::fail(fmt::format(
                "Found `INSERT` keyword not as the query root.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          }

          if (!next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected `INTO` after `INSERT` but got nothing.\nAfter token "
                "{} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(next.value(),
                                             Token{Keyword{KeywordE::INTO}})) {
            return cpp::fail(fmt::format(
                "Expected `INTO` after `INSERT` but got `{}`.\nAfter token {} "
                "(Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
          }

          if (ctx == Context::Unknown &&
              same_variant_and_value(next.value(),
                                     Token{Keyword{KeywordE::INTO}})) {
            ctx = Context::InsertE;
          }
        } break;
        case KeywordE::TABLE: {
          if (ctx == CreateTableE) {
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `DATABASE.TABLE` identifier after "
                              "`CREATE TABLE` but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              to_string(*curr), token_number, original));
            } else if (!same_variant(next.value(),
                                     Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE.TABLE` identifier after `CREATE TABLE` "
                  "but got `{}`.\nAfter token {} (Pos: {}) in query:\n    "
                  "\"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            }

          } else if (ctx == DeleteTableE) {
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `DATABASE.TABLE` identifier after "
                              "`DELETE TABLE` but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              to_string(*curr), token_number, original));
            } else if (!same_variant(next.value(),
                                     Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE.TABLE` identifier after `DELETE TABLE` "
                  "but got `{}`.\nAfter token {} (Pos: {}) in query:\n    "
                  "\"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            }
          }
        } break;
        case KeywordE::DATABASE: {
          std::cout << "Got database keyword\n";
          if (ctx == Context::CreateDatabaseE ||
              ctx == Context::ShowColumnValuesE ||
              ctx == Context::DeleteDatabaseE || ctx == Context::ShowTableDataE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE` name after `DATABASE` keyword but got "
                  "nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            } else if (!same_variant(next.value(), Token{Identifier{Name{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE` name after `DATABASE` keyword but got "
                  "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            }

            if (ctx == Context::ShowColumnValuesE) {
              std::cout << "Set variant\n";
              auto database =
                  std::get<Parser::Name>(std::get<Parser::Identifier>(*next));
              variant = {ShowColumnValues{"", database.value, ""}};
            } else if (ctx == Context::ShowTableDataE){
              if (next.has_value()){

              }
              auto database = std::get<Parser::Name>(std::get<Parser::Identifier>(*next));
              variant = {Automata::ShowColumnValues{}};
            }
          }
          break;
        case KeywordE::DATABASES: {
          if (ctx == ShowDatabasesE) {
            if (prev.has_value() && next.has_value()) {
              if (!same_variant_and_value(prev.value(),
                                          Token{Keyword{KeywordE::SHOW}})) {
                return cpp::fail(fmt::format(
                    "Expected `SHOW` keyword before `DATABASES` but got "
                    "`{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*prev), to_string(*curr), token_number,
                    original));
              } else if (!same_variant_and_value(
                             next.value(), Token{Symbol{SymbolE::SEMICOLON}})) {
                return cpp::fail(fmt::format(
                    "Expected `;` after `DATABASES` but got `{}`.\nAfter token "
                    "{} (Pos: {}) in query:\n    \"{}\"",
                    to_string(*next), to_string(*curr), token_number,
                    original));
              } else {
                variant = {Automata::ShowDatabases{}};
              }
            } else
              return cpp::fail(fmt::format(
                  "Expected `SHOW DATABASES;`.\nBut got query:\n    \"{}\"",
                  original));
          }
        }
        } break;
        default:
          break;
        }

        return {};
      },
      [&](const Type &type) -> cpp::result<void, std::string> {
        if (ctx == Context::CreateTableE ) {
          if (type.variant == TypeE::STR) {
            if (!nextp1.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected name after type str{} in `CREATE TABLE` context "
                  "but got nothing\nAfter token `{}` (Pos: {}) in query:\n    "
                  "\"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            } else if (!same_variant(*nextp1, Token{Identifier{Name{}}})) {
              return cpp::fail(
                  fmt::format("Expected name after type str{} in `CREATE "
                              "TABLE` context but got `{}`\nAfter token `{}` "
                              "(Pos: {}) in query:\n    \"{}\"",
                              to_string(*next), to_string(*nextp1),
                              to_string(*curr), token_number, original));
            }
          } else {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected name after type `{}` in `CREATE TABLE` context but "
                  "got nothing\nAfter token `{}` (Pos: {}) in query:\n    "
                  "\"{}\"",
                  to_string(*curr), to_string(*curr), token_number, original));
            } else if (!same_variant(*next, Token{Identifier{Name{}}})) {
              return cpp::fail(fmt::format(
                  "Expected name after type `{}` in `CREATE TABLE` context but "
                  "got `{}`\nAfter token `{}` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), to_string(*next), to_string(*curr),
                  token_number, original));
            }
          }
        }
        return {};
      },
      [&](const Symbol &symbol) -> cpp::result<void, std::string> {
        // If the symbol is a semicolon, the next token should be a null
        // optional;
        if (symbol.variant == SymbolE::SEMICOLON) {
          if (next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected end of query after semicolon but got `{}`\nAfter "
                "token `;` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
          }
        } else if (symbol.variant == SymbolE::OPENING_PAR) {
          if (ctx == Context::CreateTableE) {
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected type after opening parenthesis in "
                              "`CREATE TABLE` context but got nothing\nAfter "
                              "token `;` (Pos: {}) in query:\n    \"{}\"",
                              token_number, original));
            } else if (!same_variant(*next, Token{Parser::Type{}})) {
              return cpp::fail(
                  fmt::format("Expected type after opening parenthesis in "
                              "`CREATE TABLE` context but got `{}`\nAfter "
                              "token `;` (Pos: {}) in query:\n    \"{}\"",
                              to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected str, uint, int, bool or double literal after "
                  "opening parenthesis in `INSERT INTO` context but got "
                  "nothing\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant(*next, Token{String{}}) &&
                       !same_variant(*next, Token{Numbers{Int{}}}) &&
                       !same_variant(*next, Token{Numbers{UInt{}}}) &&
                       !same_variant(*next, Token{Numbers{Double{}}}) &&
                       !same_variant(*next, Token{Bool{}})) {
              return cpp::fail(fmt::format(
                  "Expected str, uint, int, bool or double literal after "
                  "opening parenthesis in `INSERT INTO` context but got "
                  "`{}`\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          }
        } else if (symbol.variant == SymbolE::COMA) {
          if (ctx == Context::CreateTableE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected type or `)` in `CREATE TABLE` context but got "
                  "nothing\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                       !same_variant(*next, Token{Parser::Type{}})) {
              return cpp::fail(fmt::format(
                  "Expected type or `)` in `CREATE TABLE` context but got "
                  "`{}`\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected value or `)` in `INSERT INTO` context but got "
                  "nothing\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                       !same_variant(*next, Token{String{}}) &&
                       !same_variant(*next, Token{Numbers{Int{}}}) &&
                       !same_variant(*next, Token{Numbers{UInt{}}}) &&
                       !same_variant(*next, Token{Numbers{Double{}}}) &&
                       !same_variant(*next, Token{Bool{}})) {
              return cpp::fail(fmt::format(
                  "Expected value or `)` in `INSERT INTO` context but got "
                  "`{}`\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          }
        } else if (symbol.variant == SymbolE::CLOSING_PAR) {
          if (ctx == Context::CreateTableE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `;` in `CREATE TABLE` context but got "
                  "nothing\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::SEMICOLON}})) {
              return cpp::fail(fmt::format(
                  "Expected `;` in `CREATE TABLE` context but got `{}`\nAfter "
                  "token `)` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `;` in `INSERT INTO` context but got "
                  "nothing\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::SEMICOLON}})) {
              return cpp::fail(fmt::format(
                  "Expected `;` in `INSERT INTO` context but got `{}`\nAfter "
                  "token `)` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          }
        }
        return {};
      },
      [&](const String &string) -> cpp::result<void, std::string> {
        if (ctx == Context::InsertE ) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `)` or `,` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                     !same_variant_and_value(*next,
                                             Token{Symbol{SymbolE::COMA}})) {
            return cpp::fail(fmt::format(
                "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Insert>(variant.value());
          var.values.push_back({string});
        } else if (ctx == Context::WhereE) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `;`, `AND` or `OR` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
              *next, Token{Symbol{SymbolE::SEMICOLON}}) &&
                     !same_variant_and_value(*next,
                                             Token{Operator{OperatorE::AND}}) &&
                     !same_variant_and_value(*next,
                                             Token{Operator{OperatorE::OR}})) {
            return cpp::fail(fmt::format(
                "Expected `;`, `AND` or `OR`  but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Show_Select>(variant.value());
          auto operador = std::get<Operator>(*prev);
          auto name_sub = std::get<Parser::NameAndSub>(std::get<Parser::Identifier>(*prevm1));
          var.restrictions.emplace_back(name_sub, operador.variant, string);
        }

        return {};
      },
      [&](const Numbers &numbers) -> cpp::result<void, std::string> {
        if (ctx == Context::InsertE) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `)` or `,` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
                         *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                     !same_variant_and_value(*next,
                                             Token{Symbol{SymbolE::COMA}})) {
            return cpp::fail(fmt::format(
                "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Insert>(variant.value());
          if (same_variant(numbers, Token{Numbers{Int{}}})) {
            auto value = std::get<Int>(numbers);
            var.values.push_back({value});
          } else if (same_variant(numbers, Token{Numbers{UInt{}}})) {
            auto value = std::get<UInt>(numbers);
            var.values.push_back({value});
          } else {
            auto value = std::get<Double>(numbers);
            var.values.push_back({value});
          }
        } else if (ctx == Context::WhereE) {
          if (!next.has_value()) {
            return cpp::fail(
                fmt::format("Expected `;`, `AND` or `OR` but got nothing.\nAfter token "
                            "{} (Pos: {}) in query:\n    \"{}\"",
                            to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(
              *next, Token{Symbol{SymbolE::SEMICOLON}}) &&
                     !same_variant_and_value(*next,
                                             Token{Operator{OperatorE::AND}}) &&
                     !same_variant_and_value(*next,
                                              Token{Operator{OperatorE::OR}})) {
            return cpp::fail(fmt::format(
                "Expected `;`, `AND` or `OR`  but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
          }

          auto &var = std::get<Automata::Show_Select>(variant.value());
          auto operador = std::get<Operator>(*prev);
          auto name_sub = std::get<Parser::NameAndSub>(std::get<Parser::Identifier>(*prevm1));

          if (same_variant(numbers, Token{Numbers{Int{}}})) {
            auto value = std::get<Int>(numbers);
            var.restrictions.emplace_back(name_sub, operador.variant, value);
          } else if (same_variant(numbers, Token{Numbers{UInt{}}})) {
            auto value = std::get<UInt>(numbers);
            var.restrictions.emplace_back(name_sub, operador.variant, value);
          } else {
            auto value = std::get<Double>(numbers);
            var.restrictions.emplace_back(name_sub, operador.variant, value);
          }
        }

        return {};
      },
      [&](const Identifier &identifier) -> cpp::result<void, std::string> {
        if (ctx == Context::CreateDatabaseE ||
            ctx == Context::DeleteDatabaseE || ctx == Context::DeleteTableE ||
            ctx == Context::ShowDatabaseE || ctx == Context::ShowTableE) {
          // If there is not a next token it is an error, there should be a
          // semicolon
          if (!next.has_value())
            return cpp::fail(fmt::format(
                "Expected semicolon after identifier but got nothing.\nAfter "
                "token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
          if (!same_variant_and_value(next.value(),
                                      Token{Symbol{SymbolE::SEMICOLON}}))
            return cpp::fail(fmt::format(
                "Expected semicolon after identifier but got `{}`.\nAfter "
                "token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
        }

        if (ctx == Context::SelectE){
          if ( std::holds_alternative<NameAndSub>(identifier) ) {
            if (!next.has_value()){
              return cpp::fail(fmt::format(
                  "Expected `table.row` OR `,` identifier but got nothing.\nAfter "
                  "token {} (Pos: {}) in query:\n    \"{}\"",
                  token_number, to_string(*curr), original));
            }
            else if(same_variant_and_value(next.value(),Token{Symbol{SymbolE::COMA}}) ||
                    same_variant_and_value(next.value(), Token{Keyword{KeywordE::WHERE}})){
              auto &var = std::get<Automata::Show_Select>(variant.value());
              auto table_column = std::get<NameAndSub>(std::get<Parser::Identifier>(*curr));
              var.tables.push_back(table_column);
              if (same_variant_and_value(next.value(), Token{Keyword{KeywordE::WHERE}})){
                ctx = Context::WhereE;
              }
            } else {
              return cpp::fail(fmt::format(
                  "Expected `WHERE` or `,` after `TABLE.COLUMN` identifier but got nothing.\nAfter "
                  "token {} (Pos: {}) in query:\n    \"{}\"",
                  token_number, to_string(*curr), original));
            }
          }


        }

        if (ctx == Context::ShowColumnValuesE && same_variant(*curr, Token{Identifier{NameAndSub{}}})) {
          if (!next.has_value())
            return cpp::fail(fmt::format(
                "Expected `;` after `TABLE.COLUMN` identifier but got nothing.\nAfter "
                "token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
          else if (!same_variant_and_value(next.value(),
                                      Token{Symbol{SymbolE::SEMICOLON}}))
            return cpp::fail(fmt::format(
                "Expected `;` after `TABLE.COLUMN` identifier but got `{}`.\nAfter "
                "token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
        } else if (ctx == Context::ShowDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::ShowDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: "
                "{}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::ShowTableE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::ShowTable{data.name, data.sub}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected table name but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::CreateDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::CreateDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: "
                "{}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::DeleteDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::DeleteDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: "
                "{}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::DeleteTableE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::DeleteTable{data.name, data.sub}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected table name but got `{}`.\nAfter token {} (Pos: {}) "
                "in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::CreateTableE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            if (!prev.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `CREATE TABLE` but got nothing.\nBefore token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(
                           *prev, Token{Keyword{KeywordE::TABLE}})) {
              return cpp::fail(fmt::format(
                  "Expected `TABLE` but got `{}`.\nBefore token {} (Pos: {}) "
                  "in query:\n    \"{}\"",
                  to_string(*prev), to_string(*curr), token_number, original));
            }

            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `(` but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::OPENING_PAR}})) {
              return cpp::fail(fmt::format(
                  "Expected `(` but got `{}`.\nAfter token {} (Pos: {}) in "
                  "query:\n    \"{}\"",
                  to_string(*next), token_number, to_string(*curr), original));
            }

            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::CreateTable{data.name, data.sub, {}}};
          }

          if (std::holds_alternative<Name>(identifier)) {
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `)` or `,` but got nothing.\nAfter "
                              "token {} (Pos: {}) in query:\n    \"{}\"",
                              to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::CLOSING_PAR}}) &&
                       !same_variant_and_value(*next,
                                               Token{Symbol{SymbolE::COMA}})) {
              return cpp::fail(fmt::format(
                  "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) "
                  "in query:\n    \"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            }

            if (same_variant(*prev, Token{Parser::Type{}})) {
              auto &var = std::get<Automata::CreateTable>(variant.value());
              std::string name = std::get<Name>(identifier).value;
              Parser::Type type = std::get<Parser::Type>(*prev);
              var.columns.insert(name, Layout::from_parser_type(type));
            } else if (same_variant_and_value(
                           *prevm1, Token{Parser::Type{TypeE::STR}})) {
              auto &var = std::get<Automata::CreateTable>(variant.value());
              std::string name = std::get<Name>(identifier).value;
              UInt size = std::get<UInt>(std::get<Numbers>(*prev));
              var.columns.insert(name, Layout{.size = size.value,
                                              .optional = false,
                                              .type = ColumnType::str});
            } else {
              return cpp::fail(fmt::format(
                  "Something went wrong while extracting types.\nIn token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            }
          }
        } else if (ctx == Context::InsertE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            if (!prev.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE.TABLE` identifier but got "
                  "nothing.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                  token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(
                           *prev, Token{Keyword{KeywordE::INTO}})) {
              return cpp::fail(fmt::format(
                  "Expected `DATABASE.TABLE` identifier but got `{}`.\nBefore "
                  "token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*prev), to_string(*curr), token_number, original));
            }

            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected `(` but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(
                           *next, Token{Symbol{SymbolE::OPENING_PAR}})) {
              return cpp::fail(fmt::format(
                  "Expected `(` but got `{}`.\nAfter token {} (Pos: {}) in "
                  "query:\n    \"{}\"",
                  to_string(*next), token_number, to_string(*curr), original));
            }

            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::Insert{data.name, data.sub, {}}};
          }
        }
        return {};
      },
      [&](const Operator &op) -> cpp::result<void, std::string> {
        if(ctx == Context::WhereE){
          if (op.variant == OperatorE::AND || op.variant == OperatorE::OR) {
            if (!prev.has_value()) {
              return cpp::fail(fmt::format(
                  "Expected value but got nothing.\nBefore token {} "
                  "(Pos: {}) in query:\n    \"{}\"",
                  token_number, to_string(*curr), original));
            } else if (!same_variant(*prev, Token{String{}}) &&
                       !same_variant(*prev, Token{Numbers{Int{}}}) &&
                       !same_variant(*prev, Token{Numbers{UInt{}}}) &&
                       !same_variant(*prev, Token{Numbers{Double{}}}) &&
                       !same_variant(*prev, Token{Bool{}})) {
              return cpp::fail(fmt::format(
                  "Expected value but got `{}`.\nBefore token {} (Pos: {}) "
                  "in query:\n    \"{}\"",
                  to_string(*prev), to_string(*curr), token_number, original));
            }

            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected table.column identifier but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              token_number, to_string(*curr), original));
            } else if (!same_variant(
                           *next, Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(fmt::format(
                  "Expected table.column identifier but got `{}`.\nAfter token {} (Pos: {}) in "
                  "query:\n    \"{}\"",
                  to_string(*next), token_number, to_string(*curr), original));
            }
          } else {
            if (!same_variant(prev.value(), Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(fmt::format(
                  "Expected `TABLE.ROW` identifier but got "
                  "nothing.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), token_number, original));
            } else if (!next.has_value()) {
              return cpp::fail(
                  fmt::format("Expected string, int, uint, bool or f64 value but got nothing.\nAfter token {} "
                              "(Pos: {}) in query:\n    \"{}\"",
                              to_string(*curr), token_number, original));
            } else if (!same_variant(*next, Token{String{}}) &&
                       !same_variant(*next, Token{Numbers{Int{}}}) &&
                       !same_variant(*next, Token{Numbers{UInt{}}}) &&
                       !same_variant(*next, Token{Numbers{Double{}}}) &&
                       !same_variant(*next, Token{Bool{}})) {
              return cpp::fail(fmt::format(
                  "Expected string, int, uint, bool or f64 value but got "
                  "{}.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), to_string(*curr), token_number, original));
            }
          }
        }
        return {}; },
      [&](const Parser::Unknown &unknown) -> cpp::result<void, std::string> {
        std::string result = Logger::show_ln(
            LOG_TYPE_::NONE,
            fmt::format("Found unknown token `{}`.", to_string(unknown)));
        result +=
            Logger::show(LOG_TYPE_::NONE,
                         fmt::format("Token (Pos: {}) in query:\n    \"{}\"",
                                     token_number, original));
        return cpp::fail(result);
      }};

  for (token_number = 0; token_number < in.size(); token_number++) {
    curr = in[token_number];

    if (token_number > 0) {
      prev = in[token_number - 1];
    } else {
      prev = std::nullopt;
    }

    if (token_number - 1 > 0) {
      prevm1 = in[token_number - 2];
    } else {
      prevm1 = std::nullopt;
    }

    if (token_number < in.size() - 1) {
      next = in[token_number + 1];
    } else {
      next = std::nullopt;
    }

    if (token_number + 1 < in.size() - 1) {
      nextp1 = in[token_number + 2];
    } else {
      nextp1 = std::nullopt;
    }

    auto result = std::visit(visitor, *curr);
    if (result.has_error()) {
      return cpp::fail(result.error());
    }
  }

  if (variant.has_value()) {
    return variant.value();
  } else {
    return cpp::fail("Unknown query.");
  }
}

std::string Automata::val_to_string(
    const std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double, Parser::Bool> &value) {
  if (std::holds_alternative<Parser::UInt>(value)) {
    return std::to_string(std::get<Parser::UInt>(value).value);
  } else if (std::holds_alternative<Parser::Int>(value)) {
    return std::to_string(std::get<Parser::Int>(value).value);
  } else if (std::holds_alternative<Parser::Double>(value)) {
    return std::to_string(std::get<Parser::Double>(value).value);
  } else if (std::holds_alternative<Parser::String>(value)) {
    return std::get<Parser::String>(value).value;
  } else if (std::holds_alternative<Parser::Bool>(value)) {
    return std::to_string(std::get<Parser::Bool>(value).value);
  }
  return "Err";
}
