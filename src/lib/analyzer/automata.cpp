#include "automata.hpp"

#include "../logger.hpp"
#include "../table.hpp"
#include "../database.hpp"
#include "parser.hpp"


cpp::result<Automata::Action, std::string> Automata::get_action_struct(std::vector<Parser::Token> in, std::string original) {
  using namespace Parser;

  Context ctx = Context::Unknown;
  std::optional<Action> variant = std::nullopt;
  size_t token_number = 0;
  std::optional<Token> next = std::nullopt;
  std::optional<Token> curr = std::nullopt;
  std::optional<Token> prev = std::nullopt;

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
        
          case KeywordE::INSERT:
          {
            if (token_number != 0) {
              return cpp::fail(
                  fmt::format(
                      "Found `INSERT` keyword not as the query root.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                      to_string(*curr), token_number, original));          
            }
            
          }
            break;
          default:
            break;
        }

        return {};
      },
      [&](const Parser::Type &type) -> cpp::result<void, std::string>  {
        if (ctx == Context::CreateTableE) {
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
          } 
        } else if (symbol.variant == SymbolE::COMA) {
          if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expected type or `)` in `CREATE TABLE` context but got nothing\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                token_number, original));
          } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant(*next, Token{Parser::Type{}})) {  
              return cpp::fail(fmt::format(
                "Expected type or `)` in `CREATE TABLE` context but got `{}`\nAfter token `,` (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, original));
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
          }
        }
        return {};
      },
      [&](const String &string) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Numbers &numbers) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Identifier &identifier) -> cpp::result<void, std::string> {
        if (ctx == Context::CreateDatabaseE || ctx == Context::DeleteDatabaseE || ctx == Context::DeleteTableE) {
          // If there is not a next token it is an error, there should be a semicolon
          if (!next.has_value())
            return cpp::fail(fmt::format(
                "Expected semicolon after database name but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                token_number, to_string(*curr), original));
          if (!same_variant_and_value(next.value(), Token{Symbol{SymbolE::SEMICOLON}}))
            return cpp::fail(fmt::format(
                "Expected semicolon after database name but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), token_number, to_string(*curr), original));
        }

        if (ctx == Context::CreateDatabaseE) {
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
            std::cout << same_variant_and_value(*next, Token{Symbol{SymbolE::COMA}}) << std::endl;
            if (!next.has_value()) {
              return cpp::fail(fmt::format(
                "Expexted `)` or `,` but got nothing.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*curr), token_number, original));
            } else if (!same_variant_and_value(*next, Token{Symbol{SymbolE::CLOSING_PAR}}) && !same_variant_and_value(*next, Token{Symbol{SymbolE::COMA}})) {
              return cpp::fail(fmt::format(
                "Expexted `)` or `,` but got `{}`.\nAfter token {} (Pos: {}) in query:\n    \"{}\"",
                to_string(*next), to_string(*curr), token_number, original));
            }
          
            auto & var = std::get<Automata::CreateTable>(variant.value());
            std::string name = std::get<Name>(identifier).value;
            Parser::Type type = std::get<Parser::Type>(*prev);
            var.columns.insert({name, type});
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

  for (; token_number < in.size(); token_number++) {
    if (token_number != 0) {
      prev = curr;
    } else {
      prev = std::nullopt;
    }
    curr = in[token_number];
    if (token_number != in.size() - 1) {
      next = in[token_number + 1];
    } else {
      next = std::nullopt;
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
