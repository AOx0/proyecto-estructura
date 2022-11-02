#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <variant>
#include <sstream>

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

  inline std::string to_string(const OperatorE& x) {
    std::ostringstream ss;
    switch (x) {
      case AND: ss << "AND"; break;
      case OR: ss << "OR"; break;
      case EQUAL: ss << "EQUAL"; break;
      case GREAT_EQUAL: ss << "GREAT_EQUAL"; break;
      case LESS_EQUAL: ss << "LESS_EQUAL"; break;
      case NOT_EQUAL: ss << "NOT_EQUAL"; break;
    }

    return ss.str();
  }

  struct Operator {
    OperatorE variant;
  };

  inline std::string to_string(const Operator& x) {
    return to_string(x.variant);
  }

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
    UN,
    SHOW
  };

  inline std::string to_string(const KeywordE& x) {
    std::ostringstream ss;

    switch (x) {
      case KeywordE::CREATE:
        ss << "CREATE";
        break;
      case KeywordE::DATABASE:
        ss << "DATABASE";
        break;
      case KeywordE::TABLE:
        ss << "TABLE";
        break;
      case KeywordE::INSERT:
        ss << "INSERT";
        break;
      case KeywordE::INTO:
        ss << "INTO";
        break;
      case KeywordE::VALUES:
        ss << "VALUES";
        break;
      case KeywordE::SELECT:
        ss << "SELECT";
        break;
      case KeywordE::FROM:
        ss << "FROM";
        break;
      case KeywordE::WHERE:
        ss << "WHERE";
        break;
      case KeywordE::UPDATE:
        ss << "UPDATE";
        break;
      case KeywordE::SET:
        ss << "SET";
        break;
      case KeywordE::DELETE:
        ss << "DELETE";
        break;
      case KeywordE::DROP:
        ss << "DROP";
        break;
      case KeywordE::PK:
        ss << "PK";
        break;
      case KeywordE::UN:
        ss << "UN";
        break;
      case KeywordE::SHOW:
        ss << "SHOW";
        break;
    }

    return ss.str();
  }

  struct Keyword {
    KeywordE variant;
  };

  inline std::string to_string(const Keyword& x) {
    return to_string(x.variant);
  }

  enum TypeE {
    I8, I16, I32, I64, U8,
    U16, U32, U64, F64, BOOL, STR
  };

  inline std::string to_string(const TypeE& x) {
    std::ostringstream ss;

    switch (x) {
      case TypeE::I8:
        ss << "i8";
        break;
      case TypeE::I16:
        ss << "i16";
        break;
      case TypeE::I32:
        ss << "i32";
        break;
      case TypeE::I64:
        ss << "i64";
        break;
      case TypeE::U8:
        ss << "u8";
        break;
      case TypeE::U16:
        ss << "u16";
        break;
      case TypeE::U32:
        ss << "u32";
        break;
      case TypeE::U64:
        ss << "u64";
        break;
      case TypeE::F64:
        ss << "f64";
        break;
      case TypeE::BOOL:
        ss << "bool";
        break;
      case TypeE::STR:
        ss << "str";
        break;
    }

    return ss.str();
  }

  // << operator calls to_string
  inline std::ostream& operator<<(std::ostream& os, const TypeE& x) {
    os << to_string(x);
    return os;
  }

  struct Type {
    TypeE variant;
  };

  inline std::string to_string(const Type& x) {
    return to_string(x.variant);
  }

  // << operator calls to_string
  inline std::ostream& operator<<(std::ostream& os, const Type& x) {
    os << to_string(x);
    return os;
  }

  enum SymbolE {
    COMA,
    SEMICOLON,
    ALL,
    CLOSING_PAR,
    OPENING_PAR
  };

  inline std::string to_string(const SymbolE& x) {
    std::ostringstream ss;

    switch (x) {
      case SymbolE::COMA:
        ss << ",";
        break;
      case SymbolE::SEMICOLON:
        ss << ";";
        break;
      case SymbolE::ALL:
        ss << "*";
        break;
      case SymbolE::CLOSING_PAR:
        ss << ")";
        break;
      case SymbolE::OPENING_PAR:
        ss << "(";
        break;
    }

    return ss.str();
  }

  struct Symbol {
    SymbolE variant;
  };

  inline std::string to_string(const Symbol& x) {
    return to_string(x.variant);
  }

  struct String {
    std::string value;
  };

  inline std::string to_string(const String& x) {
    return x.value;
  }

  struct Double {
    double value;
  };

  inline std::string to_string(const Double& x) {
    return std::to_string(x.value);
  }

  struct Name {
    std::string value;
  };

  inline std::string to_string(const Name& x) {
    return x.value;
  }

  struct Unknown {
    std::string value;
  };

  inline std::string to_string(const Unknown& x) {
    return x.value;
  }

  struct NameAndSub {
    std::string name;
    std::string sub;
  };

  inline std::string to_string(const NameAndSub& x) {
    return x.name + "." + x.sub;
  }

  using Identifier = std::variant<Name, NameAndSub>;

  inline std::string to_string(const Identifier& x) {
    return std::visit([](auto&& arg) { return to_string(arg); }, x);
  }

  struct Int {
    std::uint64_t value;
  };

  inline std::string to_string(const Int& x) {
    return std::to_string(x.value);
  }

  using Numbers = std::variant<Double, Int>;


  inline std::string to_string(const Numbers& x) {
    return std::visit([](auto&& arg) { return to_string(arg); }, x);
  }

  using Token = std::variant<Operator, Keyword, Type, Symbol, String, Numbers, Identifier, Unknown>;

  template <typename T>
  bool is_same_variant(const T& x, const T& y) {
    return x.index() == y.index();
  }

  inline std::string to_string(const Token& x) {
    return std::visit([](auto&& arg) { return to_string(arg); }, x);
  }

  std::vector<Token> parse(const std::string &in);

  bool operator==(std::vector<Token> left, std::vector<Token> right);

  bool same_variant(const Token & left, const Token & right);

  bool same_variant_and_value(const Token & left, const Token & right);

}

#endif //PARSER_HPP
