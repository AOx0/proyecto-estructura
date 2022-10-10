#include "tokenizer.hpp"

#include <regex>
#include <fmt/core.h>

std::vector<std::string> Tokenizer::tokenize(const std::string &in) {
std::vector<std::string> resultado{};

  // replace tabs and new lines by spaces
  std::string str = in;
  for (auto &c : str) {
    if (c == '\t' || c == '\n') {
      c = ' ';
    }
  }

  // replace "=" with " = ", "!" with " ! ", etc
  make_tokens_explicit(str);

  // split by spaces and add to vector
  size_t i = 0;
  while (i < str.size()) {
    std::string token;
    while (i < str.size() && str[i] != ' ') {
      token += str[i];
      i++;
    }
    i++;
    if (!token.empty()) {
      resultado.push_back(token);
    }
  }

  return resultado;
}

enum TokenType {
  KEYWORD,
  IDENTIFIER,
  TYPE,
  TOKEN,
  STRING,
  NUMBER,
  OPERATOR,
  NONE
};

void Tokenizer::make_tokens_explicit(std::string &str) {

  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '=' || str[i] == '!' || str[i] == '<' || str[i] == '>' || str[i] == '(' || str[i] == ')' || str[i] == ',' || str[i] == ';') {
      str.insert(i, " ");
      str.insert(i + 2, " ");
      i += 2;
    }
  }

  // Here we rejoin equality operators that were made 'explicit'. Example "=  =" with "=="
  str = std::regex_replace(str, std::regex("=  ="), "==");
  str = std::regex_replace(str, std::regex("!  ="), "!=");
  str = std::regex_replace(str, std::regex("<  ="), "<=");
  str = std::regex_replace(str, std::regex(">  ="), ">=");

}


std::tuple<bool, std::optional<std::string>> Tokenizer::validate(const std::string &in) {
  const static std::vector<std::string> valid_tokens = {
      "(", ")", ";", "*"
  };

  const static std::vector<std::string> valid_operators = {
      "==", ">", "<", "!", "<=", ">=", ",", "&&", "||", "AND", "0R"
  };

  const static std::vector<std::string> valid_keywords = {
      "CREATE", "DATABASE", "TABLE", "INSERT", "INTO", "VALUES",
      "SELECT", "FROM", "WHERE", "UPDATE", "SET", "DELETE", "DROP", "PK", "UN"
  };

  const static std::vector<std::string> valid_types = {
      "i8", "i16", "i32", "i64", "u8",
      "u16", "u32", "u64", "str", "f32", "f64", "bool"
  };

  auto tokens = Tokenizer::tokenize(in);
  // Check token by token if it is in the
  // list of valid tokens

  auto last_type = TokenType::NONE;
  auto current_type = TokenType::NONE;

  for (auto &token : tokens) {
    bool found_in_tokens = false;
    for (auto &valid_token : valid_tokens) {
      if (token == valid_token) {
        found_in_tokens = true;
        break;
      }
    }

    bool found_in_operators = false;
    for (auto &valid_operator : valid_operators) {
      if (token == valid_operator) {
        found_in_operators = true;
        break;
      }
    }

    bool found_in_keywords = false;
    for (auto &valid_keyword : valid_keywords) {
      if (token == valid_keyword) {
        found_in_keywords = true;
        break;
      }
    }

    bool found_in_types = false;
    for (auto &valid_type : valid_types) {
      if (token == valid_type) {
        found_in_types = true;
        break;
      }
    }

    if (found_in_tokens) {
      current_type = TokenType::TOKEN;
    } else if (found_in_operators) {
      current_type = TokenType::OPERATOR;
    } else if (found_in_keywords) {
      current_type = TokenType::KEYWORD;
    } else if (found_in_types) {
      current_type = TokenType::TYPE;
    } else if (std::regex_match(token, std::regex("(?=.)([+-]?([0-9]*)(\\.([0-9]+))?)"))) {
      current_type = TokenType::NUMBER;
    } else if (std::regex_match(token, std::regex("\".*\""))) {
      current_type = TokenType::STRING;
    } else if (std::regex_match(token, std::regex("[a-zA-Z_][a-zA-Z0-9_]*"))) {
      current_type = TokenType::IDENTIFIER;
    } else {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token: {}", token)));
    }

    // It is invalid if we have to operators together
    if (current_type == TokenType::OPERATOR && last_type == TokenType::OPERATOR) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid found two operators: {} {}", token, token)));
    }

    // It is invalid if we have two identifiers together
    if (current_type == TokenType::IDENTIFIER && last_type == TokenType::IDENTIFIER) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, found two identifiers: {} {}", token, token)));
    }

    // It is invalid if we have two types together
    if (current_type == TokenType::TYPE && last_type == TokenType::TYPE) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, found two types: {} {}", token, token)));
    }

    // It is invalid if we have two strings together
    if (current_type == TokenType::STRING && last_type == TokenType::STRING) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, found two strings: {} {}", token, token)));
    }

    // It is invalid if we have two tokens together
    if (current_type == TokenType::TOKEN && last_type == TokenType::TOKEN) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, found two tokens: {} {}", token, token)));
    }

    // It is invalid if we have two numbers together
    if (current_type == TokenType::NUMBER && last_type == TokenType::NUMBER) {
      std::cout << "Invalid token, found two numbers: " << token << std::endl;
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, found two numbers: {} {}", token, token)));
    }

    // It is invalid if we have an operator followed by a non value or identifier token
    if (last_type == TokenType::OPERATOR && !( current_type == TokenType::IDENTIFIER || current_type == TokenType::NUMBER || current_type == TokenType::STRING)) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, expected literal (number, string) or identifier after an operator: {}", token)));
    }

    // It is invalid if we have an operator preceded by a non value or identifier token
    if (current_type == TokenType::OPERATOR && !( last_type == TokenType::IDENTIFIER || last_type == TokenType::NUMBER || last_type == TokenType::STRING)) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, expected literal (number, string) or identifier before an operator: {}", token)));
    }

    // If last is a type and current is not an identifier or a str or a number then it is invalid
    if (last_type == TokenType::TYPE && current_type != TokenType::IDENTIFIER) {
      return  std::make_tuple(false, std::make_optional(fmt::format("Invalid token, expected identifier after type: {}", token)));
    }

    last_type = current_type;
  }

  return  std::make_tuple(true, std::nullopt);
}
