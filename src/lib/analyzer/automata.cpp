#include "automata.hpp"

#include "../logger.hpp"


cpp::result<std::optional<std::variant<Automata::CreateDatabase>>, std::string> Automata::get_action_struct(std::vector<Parser::Token> in, std::string original) {
  Context ctx = Context::Unknown;
  size_t token_number = 1;
  std::optional<Parser::Token> next = std::nullopt;
  std::optional<Parser::Token> curr = std::nullopt;
  std::optional<Parser::Token> prev = std::nullopt;

  auto visitor = overload{
      [&](const Parser::Keyword &keyword) -> cpp::result<void, std::string> {
        if (Context::Unknown == ctx) {
          switch (keyword.variant) {
            case Parser::KeywordE::CREATE:
              ctx = Context::CreateDatabaseE;
              break;
            default:
              break;
          }
        }

        switch (keyword.variant) {
          case Parser::KeywordE::CREATE:
          {
            if (next.has_value() && std::holds_alternative<Parser::Keyword>(*next)
                && std::get<Parser::Keyword>(*next).variant != Parser::KeywordE::DATABASE &&
                  std::get<Parser::Keyword>(*next).variant != Parser::KeywordE::TABLE) {
              std::string result = Logger::show(LOG_TYPE_::ERROR, "Expected DATABASE or TABLE after keyword CREATE.");
              result += Logger::show(LOG_TYPE_::NONE, fmt::format("After token CREATE (Pos: {}) in query:\n    \"{}\"", token_number, original));
              return cpp::fail(result);
            } else {
              token_number++;
            }
          }
            break;
          case Parser::KeywordE::DELETE:
          {
            if (next.has_value() && std::holds_alternative<Parser::Keyword>(*next)
                && std::get<Parser::Keyword>(*next).variant != Parser::KeywordE::DATABASE &&
                  std::get<Parser::Keyword>(*next).variant != Parser::KeywordE::TABLE) {
              std::string result = Logger::show(LOG_TYPE_::ERROR, "Expected DATABASE or TABLE after keyword DELETE.");
              result += Logger::show(LOG_TYPE_::NONE, fmt::format("After token DELETE (Pos: {}) in query:\n    \"{}\"", token_number, original));
              return cpp::fail(result);
            } else {
              token_number++;
            }
          }
            break;
          default:
            break;
        }

        return {};
      },
      [&](const Parser::Type &type) -> cpp::result<void, std::string>  {
        return {};
      },
      [&](const Parser::Symbol &symbol) -> cpp::result<void, std::string>  {
        return {};
      },
      [&](const Parser::String &string) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Parser::Numbers &numbers) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Parser::Identifier &identifier) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Parser::Operator &op) -> cpp::result<void, std::string> {
        return {};
      },
      [&](const Parser::Unknown &unknown) -> cpp::result<void, std::string> {
        std::string result = Logger::show(LOG_TYPE_::ERROR, "Found unknown token.");
        result += Logger::show(LOG_TYPE_::NONE, fmt::format("Token (Pos: {}) in query:\n    \"{}\"", token_number, original));
        return cpp::fail(result);
      }
  };

  for (token_number = 0; token_number < in.size(); token_number++) {
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

  return {std::nullopt};
}