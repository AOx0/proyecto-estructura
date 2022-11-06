#include "serializer.hpp"

std::string to_string(ColumnType type) {
  switch (type) {
  case ColumnType::u8:
    return "u8";
  case ColumnType::u16:
    return "u16";
  case ColumnType::u32:
    return "u32";
  case ColumnType::u64:
    return "u64";
  case ColumnType::i8:
    return "i8";
  case ColumnType::i16:
    return "i16";
  case ColumnType::i32:
    return "i32";
  case ColumnType::i64:
    return "i64";
  case ColumnType::f32:
    return "f32";
  case ColumnType::f64:
    return "f64";
  case ColumnType::str:
    return "str";
  case ColumnType::rbool:
    return "bool";
  }

  throw std::runtime_error("Invalid column type");
}