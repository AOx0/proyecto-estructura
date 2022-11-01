#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <map>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <result.hpp>
#include <optional>

#include "analyzer/parser.hpp"
#include "linkedList.hpp"

enum ColumnType {
  u8 = 1,
  u16 = 2,
  u64 = 3,
  i8 = 4,
  i16 = 5,
  i64 = 6,
  f32 = 7,
  f64 = 8,
  str = 9
};

struct Layout {
  uint64_t size;
  bool optional;
  ColumnType type;

  bool operator==(Layout const &other) const;
  
  bool operator!=(Layout const &other) const {
    return !(other == *this);
  }

  static Layout from_parser_type(Parser::Type const &type) {
    switch (type.variant) {
      case Parser::TypeE::U8:
        return Layout{.size = 1, .optional = false, .type = ColumnType::u8};
      case Parser::TypeE::U16:
        return Layout{.size = 2, .optional = false, .type = ColumnType::u16};
      case Parser::TypeE::U32:
        return Layout{.size = 4, .optional = false, .type = ColumnType::u64};
      case Parser::TypeE::I8:
        return Layout{.size = 1, .optional = false, .type = ColumnType::i8};
      case Parser::TypeE::I16:
        return Layout{.size = 2, .optional = false, .type = ColumnType::i16};
      case Parser::TypeE::I32:
        return Layout{.size = 4, .optional = false, .type = ColumnType::i64};
      //case Parser::TypeE::F32:
      //  return Layout{.size = 4, .optional = false, .type = ColumnType::f32};
      case Parser::TypeE::F64:
        return Layout{.size = 8, .optional = false, .type = ColumnType::f64};
      case Parser::TypeE::STR:
        return Layout{.size = 0, .optional = false, .type = ColumnType::str};
      default:
        throw std::runtime_error("Invalid type");
    }
  }

  friend std::ostream &operator<<(std::ostream &os, Layout const &layout) {
    os << "Layout{size=" << layout.size << ", type=" << layout.type << "}";
    return os;
  }
};

struct TableInstance {
  KeyValueList<std::string, std::tuple<Layout,void*>> data;

  /*TableInstance(std::string table_name) : data(KeyValueList<std::string, std::tuple<Layout,void*>>()) {}

  };*/
};

struct Table {
  std::optional<TableInstance> instance;
  KeyValueList<std::string, Layout> rows;
  std::shared_mutex mtx_;

  // Move constructor
  // Move constructors should be marked with except
  Table(Table &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mtx_);
    rows = std::move(rhs.rows);
  }

  // Move operator
  Table &operator=(Table &&rhs) noexcept {
    if (this != &rhs) {
      std::unique_lock lock_rhs(rhs.mtx_);
      std::unique_lock lock_this(mtx_);
      std::lock(lock_rhs, lock_this);
      rows = std::move(rhs.rows);
    }

    return *this;
  }

  [[nodiscard]] std::vector<std::uint8_t> into_vec();

  static Table from_vec(const std::vector<std::uint8_t> &in);

  static Table from_file(std::string const &path);

  void to_file(const std::string &path);

  bool operator==(Table const &other) const;

  Table(KeyValueList<std::string, Layout> &layout) : rows(std::move(layout)), mtx_() {}

  static cpp::result<Table, std::string> createTable(std::string database, std::string &table_name, KeyValueList<std::string, Layout> &layout);
};

#endif // TABLE_HPP
