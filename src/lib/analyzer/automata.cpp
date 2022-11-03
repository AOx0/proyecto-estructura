#include "automata.hpp"

#include "../logger.hpp"
#include "../table.hpp"
#include "../database.hpp"
#include "parser.hpp"


cpp::result<Automata::Action, std::string> Automata::get_action_struct(std::vector<Parser::Token> in, std::string original) {
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
      [&](const Keyword &keyword) -> cpp::result<void, std::string> {
        switch (keyword.variant) {
        
                
          case KeywordE::CREATE:
          {
            if (token_number != 0) {
              return cpp::fail(
                  fmt::format(
                      "Found `CREATE` keyword not as the query root.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            }
            
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `TABLE` after `CREATE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                       !same_variant_and_value(next.value(), Token{Keyword{KeywordE::TABLE}})) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `TABLE` after `CREATE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*next), token_number, to_string(*curr), original));
            }

            if (ctx == Context::Unknown) {
              if (same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}})) {
                ctx = Context::CreateDatabaseE;
              } else {
                ctx = Context::CreateTableE;
              }
            }
          }
            break;
          case KeywordE::DELETE:
          {
            if (token_number != 0) {
              return cpp::fail(
                  fmt::format(
                      "Found `DELETE` keyword not as the query root.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            }
            
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `TABLE` after `DELETE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                       !same_variant_and_value(next.value(), Token{Keyword{KeywordE::TABLE}})) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `TABLE` after `DELETE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*next), token_number, to_string(*curr), original));
            }

            if (ctx == Context::Unknown) {
              if (same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}})) {
                ctx = Context::DeleteDatabaseE;
              } else {
                ctx = Context::DeleteTableE;
              }
            }
          }
            break;
        
          case KeywordE::SHOW:
          {
            if (token_number != 0) {
              return cpp::fail(
                  fmt::format(
                      "Found `SHOW` keyword not as the query root.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            }
            
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `DATABASES` or `TABLE` after `SHOW` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}}) &&
                       !same_variant_and_value(next.value(), Token{Keyword{KeywordE::TABLE}})
                       && !same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASES}})) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE` or `DATABASES` or`TABLE` after `SHOW` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*next), token_number, to_string(*curr), original));
            }

            if (ctx == Context::Unknown) {
              if (same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASE}})) {
                ctx = Context::ShowDatabaseE;
              }
              else if (same_variant_and_value(next.value(), Token{Keyword{KeywordE::DATABASES}})) {
                ctx = Context::ShowDatabasesE;
              }
              else {
                ctx = Context::ShowTableE;
              }

            }
          }
            break;
        
          case KeywordE::INTO:
          {
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE.TABLE` identifier after `INTO` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            } else if (!same_variant(*next, Token{Identifier{NameAndSub{}}})) {
              return cpp::fail(
                  fmt::format(
                      "Expected `DATABASE.TABLE` identifier after `INTO` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*next), token_number, to_string(*curr), original));
            }
          }
            break;
          case KeywordE::INSERT:
          {
            if (token_number != 0) {
              return cpp::fail(
                  fmt::format(
                      "Found `INSERT` keyword not as the query root.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            }
            
            if (!next.has_value()) {
              return cpp::fail(
                  fmt::format(
                      "Expected `INTO` after `INSERT` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(next.value(), Token{Keyword{KeywordE::INTO}})) {
              return cpp::fail(
                  fmt::format(
                      "Expected `INTO` after `INSERT` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*next), token_number, to_string(*curr), original));
            }

            if (ctx == Context::Unknown && same_variant_and_value(next.value(), Token{Keyword{KeywordE::INTO}})) {
              ctx = Context::InsertE;
            }
          }
            break;
          case KeywordE::TABLE:
          {
            if (ctx == CreateTableE) {
              if (!next.has_value()) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE.TABLE` identifier after `CREATE TABLE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*curr), token_number, original));
              } else if (!same_variant(next.value(), Token{Identifier{NameAndSub{}}})) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE.TABLE` identifier after `CREATE TABLE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*next), to_string(*curr), token_number, original));
              }

            } else if (ctx == DeleteTableE) {
              if (!next.has_value()) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE.TABLE` identifier after `DELETE TABLE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*curr), token_number, original));
              } else if (!same_variant(next.value(), Token{Identifier{NameAndSub{}}})) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE.TABLE` identifier after `DELETE TABLE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*next), to_string(*curr), token_number, original));
              }

            }
          }
            break;
          case KeywordE::DATABASE:
          {
            if (ctx == CreateDatabaseE) {
              if (!next.has_value()) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE` name after `CREATE DATABASE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*curr), token_number, original));
              } else if (!same_variant(next.value(), Token{Identifier{Name{}}})) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE` name after `CREATE DATABASE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*next), to_string(*curr), token_number, original));
              }

            } else if (ctx == DeleteDatabaseE) {
              if (!next.has_value()) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE` name after `DELETE DATABASE` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*curr), token_number, original));
              } else if (!same_variant(next.value(), Token{Identifier{Name{}}})) {
                return cpp::fail(
                    fmt::format(
                        "Expected `DATABASE` name after `DELETE DATABASE` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*next), to_string(*curr), token_number, original));
              }
            }
          }
            break;
          case KeywordE::DATABASES:{
            if(ctx == ShowDatabaseE){
              if(prev.has_value() && next.has_value()){
                if(!same_variant_and_value(prev.value(), Token{Keyword{KeywordE::SHOW}})){
                  return cpp::fail(
                      fmt::format(
                          "Expected `SHOW` keyword before `DATABASES` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                          to_string(*prev), to_string(*curr), token_number, original));
                }
                if(!same_variant_and_value(next.value(), Token{Symbol{SymbolE::SEMICOLON}})){
                  return cpp::fail(
                      fmt::format(
                          "Expected `;` after `DATABASES` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                          to_string(*next), to_string(*curr), token_number, original));
                  }
              }
              else{
                return cpp::fail(
                    fmt::format(
                        "Expected `SHOW` or `NOTHING` keyword before or after `DATABASES` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                        to_string(*curr), token_number, original));
              }

            }
            variant = {Automata::ShowDatabases{}};
          }
            break;
          default:
            break;
        }

        return {};
      },
      [&](const Type &type) -> cpp::result<void, std::string>  {
        if (ctx == Context::CreateTableE) {  
          if (type.variant == TypeE::STR) {
            if (!nextp1.has_value()) {
              return cpp::fail(fmt::format(
                "Expected name after type str{} in `CREATE TABLE` context but got nothing\nAfter token `{}` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
            } else if (!same_variant(*nextp1, Token{Identifier{Name{}}})) {
              return cpp::fail(fmt::format(
                "Expected name after type str{} in `CREATE TABLE` context but got `{}`\nAfter token `{}` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), to_string(*nextp1), to_string(*curr), token_number, original));
            }
          } else {
            if (!next.has_value()) {
                return cpp::fail(fmt::format(
                  "Expected name after type `{}` in `CREATE TABLE` context but got nothing\nAfter token `{}` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), to_string(*curr), token_number, original));
            } else if (!same_variant(*next, Token{Identifier{Name{}}})) {
                return cpp::fail(fmt::format(
                  "Expected name after type `{}` in `CREATE TABLE` context but got `{}`\nAfter token `{}` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*curr), to_string(*next), to_string(*curr), token_number, original));
            }
          }
        }
        return {};
      },
      [&](const Symbol &symbol) -> cpp::result<void, std::string>  {
        // If the symbol is a semicolon, the next token should be a null optional;
        if (symbol.variant == SymbolE::SEMICOLON) {
          if (next.has_value()) {
            return cpp::fail(fmt::format(
                "Expected end of query after semicolon but got `{}`\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
          }
        } else if (symbol.variant == SymbolE::OPENING_PAR) {
          if (ctx == Context::CreateTableE) {
            if ( !next.has_value() ) {
              return cpp::fail(fmt::format(
                "Expected type after opening parenthesis in `CREATE TABLE` context but got nothing\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                token_number, original));
            } else if ( !same_variant(*next, Token{Parser::Type{}} )) {
              return cpp::fail(fmt::format(
                "Expected type after opening parenthesis in `CREATE TABLE` context but got `{}`\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if ( !next.has_value() ) {
              return cpp::fail(fmt::format(
                "Expected str, uint, int or double literal after opening parenthesis in `INSERT INTO` context but got nothing\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                token_number, original));
            } else if ( !same_variant(*next, Token{String{}}) 
                  && !same_variant(*next, Token{Numbers{Int{}}}) 
                  && !same_variant(*next, Token{Numbers{UInt{}}}) 
                  && !same_variant(*next, Token{Numbers{Double{}}}) 
            ) {
              return cpp::fail(fmt::format(
                "Expected str, uint, int or double literal after opening parenthesis in `INSERT INTO` context but got `{}`\nAfter token `;` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
            }
          } 
        } else if (symbol.variant == SymbolE::COMA) {
          if (ctx == Context::CreateTableE) {
            if (!next.has_value()) {
                return cpp::fail(fmt::format(
                  "Expected type or `)` in `CREATE TABLE` context but got nothing\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant(*next, Token{Parser::Type{}})) {  
                return cpp::fail(fmt::format(
                  "Expected type or `)` in `CREATE TABLE` context but got `{}`\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if (!next.has_value()) {
                return cpp::fail(fmt::format(
                  "Expected value or `)` in `INSERT INTO` context but got nothing\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) 
                  && !same_variant(*next, Token{String{}}) 
                  && !same_variant(*next, Token{Numbers{Int{}}}) 
                  && !same_variant(*next, Token{Numbers{UInt{}}}) 
                  && !same_variant(*next, Token{Numbers{Double{}}})) {  
                return cpp::fail(fmt::format(
                  "Expected value or `)` in `INSERT INTO` context but got `{}`\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                  to_string(*next), token_number, original));
            }
          }
        } else if (symbol.variant == SymbolE::CLOSING_PAR) {
          if (ctx == Context::CreateTableE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `;` in `CREATE TABLE` context but got nothing\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::SEMICOLON}})) {
              return cpp::fail(fmt::format(
                "Expected `;` in `CREATE TABLE` context but got `{}`\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
            }
          } else if (ctx == Context::InsertE) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `;` in `INSERT INTO` context but got nothing\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::SEMICOLON}})) {
              return cpp::fail(fmt::format(
                "Expected `;` in `INSERT INTO` context but got `{}`\nAfter token `)` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
            }
          
          }
        }
        return {};
      },
      [&](const String &string) -> cpp::result<void, std::string> {
        if (ctx == Context::InsertE) {
          if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `)` or `,` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant_and_value(*next, Token{Symbol{SymbolE::COMA}})) {
              return cpp::fail(fmt::format(
                "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
            }
        
            auto & var = std::get<Automata::Insert>(variant.value());  
            var.values.push_back({string});      
        }
      
        return {};
      },
      [&](const Numbers &numbers) -> cpp::result<void, std::string> {
        if (ctx == Context::InsertE) {
          if (!next.has_value()) {
            return cpp::fail(fmt::format(
              "Expected `)` or `,` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
              to_string(*curr), token_number, original));
          } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant_and_value(*next, Token{Symbol{SymbolE::COMA}})) {
            return cpp::fail(fmt::format(
              "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
              to_string(*next), to_string(*curr), token_number, original));
          }
        
          auto & var = std::get<Automata::Insert>(variant.value());  
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
          
        }
      
        return {};
      },
      [&](const Identifier &identifier) -> cpp::result<void, std::string> {
        if (ctx == Context::CreateDatabaseE || ctx == Context::DeleteDatabaseE || ctx == Context::DeleteTableE || ctx == Context::ShowDatabaseE || ctx == Context::ShowTableE) {
          // If there is not a next token it is an error, there should be a semicolon
          if (!next.has_value())
            return cpp::fail(fmt::format(
                "Expected semicolon after identifier but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
          if (!same_variant_and_value(next.value(), Token{Symbol{SymbolE::SEMICOLON}}))
            return cpp::fail(fmt::format(
                "Expected semicolon after identifier but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
        }
      
        
        if (ctx == Context::ShowDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::ShowDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::ShowTableE) {  
          if (std::holds_alternative<NameAndSub>(identifier)) {
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::ShowTable{data.name, data.sub}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected table name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::CreateDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::CreateDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::DeleteDatabaseE) {
          if (std::holds_alternative<Name>(identifier)) {
            std::string name = std::get<Name>(identifier).value;
            variant = {Automata::DeleteDatabase{name}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected database name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::DeleteTableE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::DeleteTable{data.name, data.sub}};
            return {};
          } else {
            return cpp::fail(fmt::format(
                "Expected table name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, to_string(*curr), original));
          }
        } else if (ctx == Context::CreateTableE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            if (!prev.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `CREATE TABLE` but got nothing.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(*prev, Token{Keyword{KeywordE::TABLE}})) {
              return cpp::fail(fmt::format(
                "Expected `TABLE` but got `{}`.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*prev), to_string(*curr), token_number, original));
            }
          
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `(` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::OPENING_PAR}})) {
              return cpp::fail(fmt::format(
                "Expected `(` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
            }
          
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::CreateTable{data.name, data.sub, {}}};
          }
        
          if (std::holds_alternative<Name>(identifier)) {
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `)` or `,` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant_and_value(*next, Token{Symbol{SymbolE::COMA}})) {
              return cpp::fail(fmt::format(
                "Expected `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
            }
          
            if (same_variant(*prev, Token{Parser::Type{}})) {
              auto & var = std::get<Automata::CreateTable>(variant.value());
              std::string name = std::get<Name>(identifier).value;
              Parser::Type type = std::get<Parser::Type>(*prev);
              var.columns.insert(name, Layout::from_parser_type(type));
            } else if (same_variant_and_value(*prevm1, Token{Parser::Type{TypeE::STR}})) {
              auto & var = std::get<Automata::CreateTable>(variant.value());
              std::string name = std::get<Name>(identifier).value;
              UInt size = std::get<UInt>(std::get<Numbers>(*prev));
              var.columns.insert(name, Layout{.size=size.value, .optional=false, .type=ColumnType::str});
            } else {
              return cpp::fail(fmt::format(
                "Something went wrong while extracting types.\nIn token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
            }
          }
        } else if (ctx == Context::InsertE) {
          if (std::holds_alternative<NameAndSub>(identifier)) {
            if (!prev.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `DATABASE.TABLE` identifier but got nothing.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(*prev, Token{Keyword{KeywordE::INTO}})) {
              return cpp::fail(fmt::format(
                "Expected `DATABASE.TABLE` identifier but got `{}`.\nBefore token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*prev), to_string(*curr), token_number, original));
            }
          
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected `(` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::OPENING_PAR}})) {
              return cpp::fail(fmt::format(
                "Expected `(` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
            }
          
            auto data = std::get<NameAndSub>(identifier);
            variant = {Automata::Insert{data.name, data.sub, {}}};
          }
        }
          return {};
      },
      [&](const Operator &op) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Parser::Unknown &unknown) -> cpp::result<void, std::string> {
        std::string result = Logger::show_ln(LOG_TYPE_::NONE, fmt::format("Found unknown token `{}`.", to_string(unknown)));
        result += Logger::show(LOG_TYPE_::NONE, fmt::format("Token (Pos: {}) in query:\n    \"{}\"", token_number, original));
        return cpp::fail(result);
      }
  };

  for (token_number = 0; token_number < in.size(); token_number++) {
    curr = in[token_number];
    
    if (token_number > 0) {
      prev = in[token_number-1];
    } else {
      prev = std::nullopt;
    }
    
    if (token_number - 1 > 0) {
      prevm1 = in[token_number-2];
    } else {
      prevm1 = std::nullopt;
    }
    
    if (token_number < in.size()-1) {
      next = in[token_number + 1];
    } else {
      next = std::nullopt;
    }

    if (token_number+1 < in.size()-1) {
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
