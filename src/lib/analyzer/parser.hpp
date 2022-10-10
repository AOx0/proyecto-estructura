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

  struct Operator { OperatorE variant; };

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

  struct Keyword { KeywordE variant; };

  enum TypeE {
      I8, I16, I32, I64, U8,
      U16, U32, U64, str, F32, F64, BOOL
  };

  struct Type { TypeE variant; };

  enum SymbolE {
    COMA,
    SEMICOLON,
    ASTERISK,
    CLOSING_PAR,
    OPENING_PAR
  };

  struct Symbol { SymbolE variant; };

  struct String { std::string value; };
  
  struct Float { float value; };
  struct Double { double value; };
  using Floating = std::variant<Float, Double>;
  
  struct Int { std::uint64_t value; };
  using Numbers = std::variant<Floating, Int>;
  
  using Token = std::variant<Operator, Keyword, Type, Symbol, String, Numbers>;
}

#endif //PARSER_HPP
