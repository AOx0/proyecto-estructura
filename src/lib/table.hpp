#ifndef TABLE_HPP
#define TABLE_HPP

#pragma once

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
  u32 = 11,
  u64 = 3,
  i8 = 4,
  i16 = 5,
  i32 = 10,
  i64 = 6,
  f32 = 7,
  f64 = 8,
  str = 9
};

std::string to_string(ColumnType type);

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
  
  std::string ly_to_string() {
    std::stringstream result;
    result << *this;
    return result.str();
  }

  friend std::ostream &operator<<(std::ostream &os, Layout const &layout) {
    os << to_string(layout.type) << "(" << layout.size << " bytes)";
    //os << "Layout{size=" << layout.size << ", type=" << layout.type << "}";
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
  std::string name;
  
  friend std::ostream &operator<<(std::ostream &os, Table const &table) {
    os << "Table " << table.name << ":\n";
    
    table.rows.for_each_c([&](const KeyValue<std::string, Layout> & keyval){
      os << "    " << keyval.key << " : " << keyval.value << '\n';
      return false;
    });
    
    return os;
  }

  // Move constructor
  // Move constructors should be marked with except
  Table(Table &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mtx_);
    rows = std::move(rhs.rows);
    name = std::move(rhs.name);
  }

  // Move operator
  Table &operator=(Table &&rhs) noexcept {
    if (this != &rhs) {
      std::unique_lock lock_rhs(rhs.mtx_);
      std::unique_lock lock_this(mtx_);
      std::lock(lock_rhs, lock_this);
      rows = std::move(rhs.rows);
      name = std::move(rhs.name);
    }

    return *this;
  }

  [[nodiscard]] std::vector<std::uint8_t> into_vec();

  static Table from_vec(const std::vector<std::uint8_t> &in, const std::string & name);

  static Table from_file(std::string const &path, std::string const &name);

  void to_file(const std::string &path);
  
  cpp::result<void, std::string> try_insert(List<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double>> && values) {
    // First we verify type casting safeness
    if (values.len() != rows.len()) {
      std::stringstream error;
      error 
        << "The number of fields does not match.\n" 
        << fmt::format("Attempt to insert {} elements into a {} element column\n", values.len(), rows.len())
        << "Make sure you follow the positional arguments described by:\n"
        << rows;
      return cpp::fail(error.str());
    }
    
    using Value = std::variant<
        std::uint8_t, std::uint16_t, std::uint32_t, 
        std::uint64_t, std::int8_t, std::int16_t, 
        std::int32_t, std::int64_t, std::string, double>;
    std::vector<Value> to_insert;
    
    Node<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double>> * current = values.first();
    Node<KeyValue<std::string, Layout>> * current_column = rows.first();
    for (int i=0; i<values.len(); i++) {
      using Variant = std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double>;
      
      Layout & current_layout = current_column->value.value;        
      std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double> & current_value = current->value;      
      
      // Here is where we do actually verify type matching
      if (std::holds_alternative<Parser::String>(current_value)) {
        if (current_layout.type == ColumnType::str) {
          std::string value = get<Parser::String>(current_value).value;
          to_insert.emplace_back(value);
        } else {
          return cpp::fail(
            fmt::format(
              "Cannot push string to field {} which takes a {}", 
              i, current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Double>(current_value)) {
        if (current_layout.type == ColumnType::f64) {
          double value = get<Parser::Double>(current_value).value;
          to_insert.emplace_back(value);
        } else {
          return cpp::fail(
            fmt::format(
              "Cannot push string to field {} which takes a {}", 
              i, current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::UInt>(current_value)) {
        if (current_layout.type == ColumnType::u8 ||
            current_layout.type == ColumnType::u16 ||
            current_layout.type == ColumnType::u32 ||
            current_layout.type == ColumnType::u64 ||
            current_layout.type == ColumnType::i8 ||
            current_layout.type == ColumnType::i16 ||
            current_layout.type == ColumnType::i32 ||
            current_layout.type == ColumnType::i64
         ) {
          switch (current_layout.type) {
            case ColumnType::u8:
            {
              std::uint8_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint8_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::u16:
            {
              std::uint16_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint16_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::u32:
            {
              std::uint32_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint32_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::u64:
            {
              std::uint64_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint64_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::i8:
            {
              std::int8_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int8_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::i16:
            {
              std::int16_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int16_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::i32:
            {
              std::int32_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int32_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            case ColumnType::i64:
            {
              std::int64_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() || std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int64_t>::max()) {
                return cpp::fail(
                  fmt::format(
                    "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                    i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
            break;
            default:
              return cpp::fail(
                fmt::format(
                  "Cannot push uint to field {} which takes a {}",
                  i, current_layout.ly_to_string()));
          }
        } else {
          return cpp::fail(
            fmt::format(
              "Cannot push uint to field {} which takes a {}",
              i, current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Int>(current_value)) {
        if (
          current_layout.type == ColumnType::i8 ||
          current_layout.type == ColumnType::i16 ||
          current_layout.type == ColumnType::i32 ||
          current_layout.type == ColumnType::i64
        ) {
            switch (current_layout.type) {
              case ColumnType::i8:
              {
                std::int8_t value = 0;
                std::stringstream temp;
                temp << std::get<Parser::Int>(current_value).value;
                temp >> value;
                if (temp.fail() || std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int8_t>::max() || std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int8_t>::min()) {
                  return cpp::fail(
                    fmt::format(
                      "Cannot push int to field {} which takes a {} because the value overflows",
                      i, current_layout.ly_to_string()));
                } else {
                  to_insert.emplace_back(value);
                }
              }
              break;
              case ColumnType::i16:
              {
                std::int16_t value = 0;
                std::stringstream temp;
                temp << std::get<Parser::Int>(current_value).value;
                temp >> value;
                if (temp.fail() || std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int16_t>::max() || std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int16_t>::min()) {
                  return cpp::fail(
                    fmt::format(
                      "Cannot push int to field {} which takes a {} because the value overflows",
                      i, current_layout.ly_to_string()));
                } else {
                  to_insert.emplace_back(value);
                }
              }
              break;
              case ColumnType::i32:
              {
                std::int32_t value = 0;
                std::stringstream temp;
                temp << std::get<Parser::Int>(current_value).value;
                temp >> value;
                if (temp.fail() || std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int32_t>::max() || std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int32_t>::min()) {
                  return cpp::fail(
                    fmt::format(
                      "Cannot push int to field {} which takes a {} because the value overflows",
                      i, current_layout.ly_to_string()));
                } else {
                  to_insert.emplace_back(value);
                }
              }
              break;
              case ColumnType::i64:
              {
                std::int64_t value = 0;
                std::stringstream temp;
                temp << std::get<Parser::Int>(current_value).value;
                temp >> value;
                if (temp.fail() || std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int64_t>::max() || std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int64_t>::min()) {
                  return cpp::fail(
                    fmt::format(
                      "Cannot push int to field {} which takes a {} because the value overflows",
                      i, current_layout.ly_to_string()));
                } else {
                  to_insert.emplace_back(value);
                }
              }
              break;
              default:
                return cpp::fail(
                  fmt::format(
                    "Cannot push int to field {} which takes a {}",
                    i, current_layout.ly_to_string()));
            }
        } else {
          return cpp::fail(
            fmt::format(
              "Cannot push int to field {} which takes a {}",
              i, current_layout.ly_to_string()));
        }
      } else
          return cpp::fail(fmt::format("Cannot push to field {}", i));
      
      current = current->next;
      current_column = current_column->next;
    }
    
    return {};
  }

  bool operator==(Table const &other) const;

  Table(const std::string & name, KeyValueList<std::string, Layout> &layout)
    : name(fmt::format("{}", name))
    , rows(std::move(layout))
    , mtx_()
  {}

  static cpp::result<Table, std::string> createTable(
      std::string database, 
      std::string &table_name, 
      KeyValueList<std::string, Layout> &layout
    );
};

#endif // TABLE_HPP
