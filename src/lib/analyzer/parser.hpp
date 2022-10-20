#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <variant>

#include "tokenizer.hpp"

namespace Parser {
  enum OperatorE {
    AND,
    OR,
    EQUAL,
    GREAT_EQUAL,
    LESS_EQUAL,
    NOT_EQUAL
  };

  struct Operator {
    OperatorE variant;
  };

  enum KeywordE {
    CREATE,
    DATABASE,
    TABLE,
    INSERT,
    INTO,
    VALUES,
    SELECT,
    FROM,
    WHERE,
    UPDATE,
    SET,
    DELETE,
    DROP,
    PK,
    UN
  };

  struct Keyword {
    KeywordE variant;
  };

  enum TypeE {
    I8, I16, I32, I64, U8,
    U16, U32, U64, F64, BOOL, STR
  };

  struct Type {
    TypeE variant;
  };

  enum SymbolE {
    COMA,
    SEMICOLON,
    ALL,
    CLOSING_PAR,
    OPENING_PAR
  };

  struct Symbol {
    SymbolE variant;
  };

  struct String {
    std::string value;
  };

  struct Double {
    double value;
  };

  struct Name {
    std::string value;
  };

  struct Unknown {
    std::string value;
  };

  struct NameAndSub {
    std::string name;
    std::string sub;
  };

  using Identifier = std::variant<Name, NameAndSub>;

  struct Int {
    std::uint64_t value;
  };
  using Numbers = std::variant<Double, Int>;

  using Token = std::variant<Operator, Keyword, Type, Symbol, String, Numbers, Identifier, Unknown>;

  std::vector<Token> parse(const std::string &in);

  bool operator==(std::vector<Token> left, std::vector<Token> right);
}

#endif //PARSER_HPP
