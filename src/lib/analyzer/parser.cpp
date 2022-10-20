#include "parser.hpp"
#include <regex>
#include <string>
#include <sstream>

std::vector<Parser::Token> Parser::parse(const std::string &in) {
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
      "u16", "u32", "u64", "str", "f64", "bool"
  };

  std::vector<Token> resultado{};

  std::vector<std::string> tokens = Parser::tokenize(in);

  for (auto &token: tokens) {
    // Check if token is in valid_tokens with find
    auto it = std::find(valid_tokens.begin(), valid_tokens.end(), token);
    if (it != valid_tokens.end()) {
      if (*it == "*" || *it == "ALL") {
        resultado.emplace_back(Symbol{SymbolE::ALL});
        continue;
      }
      if (*it == "(") {
        resultado.emplace_back(Symbol{SymbolE::OPENING_PAR});
        continue;
      }
      if (*it == ")") {
        resultado.emplace_back(Symbol{SymbolE::CLOSING_PAR});
        continue;
      }
      if (*it == ";") {
        resultado.emplace_back(Symbol{SymbolE::SEMICOLON});
        continue;
      }
      if (*it == ",") {
        resultado.emplace_back(Symbol{SymbolE::COMA});
        continue;
      }
    }

    // Check if token is in valid_keywords with find
    it = std::find(valid_keywords.begin(), valid_keywords.end(), token);
    if (it != valid_keywords.end()) {
#define KEYWORD(x) if (*it == #x) { resultado.emplace_back(Keyword{KeywordE::x});  continue; }
      KEYWORD(SELECT)
      KEYWORD(INSERT)
      KEYWORD(UPDATE)
      KEYWORD(DELETE)
      KEYWORD(DROP)
      KEYWORD(CREATE)
      KEYWORD(TABLE)
      KEYWORD(DATABASE)
      KEYWORD(VALUES)
      KEYWORD(FROM)
      KEYWORD(WHERE)
      KEYWORD(SET)
      KEYWORD(PK)
      KEYWORD(UN)
      KEYWORD(INTO)
#undef KEYWORD
    }

    // Check if token is in valid_operators with find
    it = std::find(valid_operators.begin(), valid_operators.end(), token);
    if (it != valid_operators.end()) {
      if (*it == "==") {
        resultado.emplace_back(Operator{OperatorE::EQUAL});
        continue;
      }
      if (*it == ">") {
        resultado.emplace_back(Operator{OperatorE::GREAT_EQUAL});
        continue;
      }
      if (*it == "<") {
        resultado.emplace_back(Operator{OperatorE::LESS_EQUAL});
        continue;
      }
      if (*it == "!=") {
        resultado.emplace_back(Operator{OperatorE::NOT_EQUAL});
        continue;
      }
      if (*it == "&&" || *it == "AND") {
        resultado.emplace_back(Operator{OperatorE::AND});
        continue;
      }
      if (*it == "||" || *it == "0R") {
        resultado.emplace_back(Operator{OperatorE::OR});
        continue;
      }
      if (*it == ">=") {
        resultado.emplace_back(Operator{OperatorE::GREAT_EQUAL});
        continue;
      }
      if (*it == "<=") {
        resultado.emplace_back(Operator{OperatorE::LESS_EQUAL});
        continue;
      }
    }

    // Check if token is in valid_types with find
    it = std::find(valid_types.begin(), valid_types.end(), token);
    if (it != valid_types.end()) {
      if (*it == "i8") {
        resultado.emplace_back(Type{TypeE::I8});
        continue;
      }
      if (*it == "i16") {
        resultado.emplace_back(Type{TypeE::I16});
        continue;
      }
      if (*it == "i32") {
        resultado.emplace_back(Type{TypeE::I32});
        continue;
      }
      if (*it == "i64") {
        resultado.emplace_back(Type{TypeE::I64});
        continue;
      }
      if (*it == "u8") {
        resultado.emplace_back(Type{TypeE::U8});
        continue;
      }
      if (*it == "u16") {
        resultado.emplace_back(Type{TypeE::U16});
        continue;
      }
      if (*it == "u32") {
        resultado.emplace_back(Type{TypeE::U32});
        continue;
      }
      if (*it == "u64") {
        resultado.emplace_back(Type{TypeE::U64});
        continue;
      }
      if (*it == "str") {
        resultado.emplace_back(Type{TypeE::STR});
        continue;
      }
      if (*it == "f64") {
        resultado.emplace_back(Type{TypeE::F64});
        continue;
      }
      if (*it == "bool") {
        resultado.emplace_back(Type{TypeE::BOOL});
        continue;
      }
    }

    // Check if token is a number
    // TODO: Figure out wtf is going on with strings to custom integer types :D
    if (std::regex_match(token, std::regex("[+-]?[0-9]+"))) {
      resultado.emplace_back(Numbers{Int{static_cast<uint64_t>(std::stoi(token))}});
      continue;
    }

    // Check if token is a string
    if (std::regex_match(token, std::regex("\".*\""))) {
      // Remove quotes from token
      token.erase(std::remove(token.begin(), token.end(), '\"'), token.end());
      resultado.emplace_back(String{token});
      continue;
    }

    // Check if token is a identifier
    if (std::regex_match(token, std::regex("([a-zA-Z_][a-zA-Z0-9_]*)"))) {
      resultado.emplace_back(Identifier{Name{token}});
      continue;
    }

    // Check if token is an identifier with sub-member
    if (std::regex_match(token, std::regex("([a-zA-Z_][a-zA-Z0-9_]*)(\\.([a-zA-Z_][a-zA-Z0-9_]*))"))) {
      // Split token in two parts by dot
      std::vector<std::string> parts;
      std::string part;
      std::istringstream ss(token);
      while (std::getline(ss, part, '.')) {
        parts.push_back(part);
      }
      resultado.emplace_back(Identifier{NameAndSub{parts[0], parts[1]}});
      continue;
    }

    // Check if token is a double
    if (std::regex_match(token, std::regex("(?=.)([+-]?([0-9]*)(\\.([0-9]+))?)"))) {
      resultado.emplace_back(Numbers{Double{std::stod(token)}});
      continue;
    }


    resultado.emplace_back(Unknown{token});

  }

  return resultado;
}

bool Parser::operator==(std::vector<Token> left, std::vector<Token> right) {
#define CMP(tipo, field) if (std::holds_alternative<tipo>(left[i]) && std::holds_alternative<tipo>(right[i])) {\
  if (std::get<tipo>(left[i]).field != std::get<tipo>(right[i]).field) {\
    return false;\
  }\
}                                                                                                    \
    // Match every token variant and compare left and right depending on the variant
  for (int i = 0; i < left.size(); i++) {
    CMP(Operator, variant)
    else CMP(Type, variant)
    else CMP(Keyword, variant)
    else CMP(Symbol, variant)
    else CMP(String, value)
    else CMP(Unknown, value)
    else if (std::holds_alternative<Identifier>(left[i]) && std::holds_alternative<Identifier>(right[i])) {
      if (std::holds_alternative<Name>(std::get<Identifier>(left[i])) &&
          std::holds_alternative<Name>(std::get<Identifier>(right[i]))) {
        if (std::get<Name>(std::get<Identifier>(left[i])).value != std::get<Name>(std::get<Identifier>(right[i])).value) {
          return false;
        } else {
          continue;
        }
      } else if (std::holds_alternative<NameAndSub>(std::get<Identifier>(left[i])) &&
                 std::holds_alternative<NameAndSub>(std::get<Identifier>(right[i]))) {
        if (std::get<NameAndSub>(std::get<Identifier>(left[i])).name != std::get<NameAndSub>(std::get<Identifier>(right[i])).name
            || std::get<NameAndSub>(std::get<Identifier>(left[i])).sub != std::get<NameAndSub>(std::get<Identifier>(right[i])).sub) {
          return false;
        } else {
          continue;
        }
      } else {
        return false;
      }
    }
    else if (std::holds_alternative<Numbers>(left[i]) && std::holds_alternative<Numbers>(right[i])) {
      if (std::holds_alternative<Int>(std::get<Numbers>(left[i])) &&
          std::holds_alternative<Int>(std::get<Numbers>(right[i]))) {
        if (std::get<Int>(std::get<Numbers>(left[i])).value != std::get<Int>(std::get<Numbers>(right[i])).value) {
          return false;
        } else {
          continue;
        }
      } else if (std::holds_alternative<Double>(std::get<Numbers>(left[i])) &&
                 std::holds_alternative<Double>(std::get<Numbers>(left[i]))) {
        if (std::get<Double>(std::get<Numbers>(left[i])).value !=
            std::get<Double>(std::get<Numbers>(left[i])).value) {
          return false;
        } else {
          continue;
        }
      }  else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}