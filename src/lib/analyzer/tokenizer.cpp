#include "tokenizer.hpp"

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
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '=' || str[i] == '!' || str[i] == '<' || str[i] == '>' || str[i] == '(' || str[i] == ')' || str[i] == ',' || str[i] == ';') {
      str.insert(i, " ");
      str.insert(i + 2, " ");
      i += 2;
    }
  }

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

bool Tokenizer::validate(const std::string &in) {
  const static std::vector<std::string> valid_tokens = {
      "CREATE", "DATABASE", "TABLE", "i8", "i16", "i32", "i64", "u8",
      "u16", "u32", "u64", "str", "f32", "f64", "bool", "INSERT", "INTO", "VALUES",
      "(", ")", ";", "*", "SELECT", "FROM", "WHERE", "AND", "OR", "=", ">", "<", "!", "<=", ">=",
      "UPDATE", "SET", "DELETE", "DROP", "PK", "UN"
  };

  auto tokens = Tokenizer::tokenize(in);
  // Check token by token if it is in the
  // list of valid tokens
  for (auto &token : tokens) {
    bool found = false;
    for (auto &valid_token : valid_tokens) {
      if (token == valid_token) {
        found = true;
        break;
      }
    }
    if (!found) {
      std::cout << "Not a keyword: " << token << std::endl;
     // Check if possible identifier or literal
      if (token[0] == '\'' || token[0] == '\"') {
        // Literal
        if (token[0] != token[token.size() - 1]) {
          std::cout << "Invalid literal: " << token << std::endl;
          return false;
        }
      } else {
        // Number
        bool is_number = true;
        for (auto &c : token) {
          if (!isdigit(c) && c != '.' && c != '-' && c != '+' && c != '_') {
            is_number = false;
            break;
          }
        }
        if (!is_number) {
          std::cout << "Invalid number: " << token << std::endl;
          return false;
        }
      }
    }
  }

  return true;
}
