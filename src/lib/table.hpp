#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <result.hpp>
#include <shared_mutex>
#include <variant>
#include <vector>
#include <chrono>
#include <thread>

#include "analyzer/parser.hpp"
#include "fm.hpp"
#include "linkedList.hpp"
#include "serializer.hpp"

struct DatabaseTable {
  KeyValueList<std::string, Layout> columns;
  std::shared_mutex mtx_;
  std::string name;

  friend std::ostream &operator<<(std::ostream &os, DatabaseTable &table) {
    std::shared_lock lock(table.mtx_);

    os << "DatabaseTable " << table.name << ":\n";

    table.columns.for_each_c([&](const KeyValue<std::string, Layout> &keyval) {
      os << "    " << keyval.key << " : " << keyval.value << '\n';
      return false;
    });

    return os;
  }

  // Move constructor
  // Move constructors should be marked with except
  DatabaseTable(DatabaseTable &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mtx_);
    columns = std::move(rhs.columns);
    name = std::move(rhs.name);
  }

  // Move operator
  DatabaseTable &operator=(DatabaseTable &&rhs) noexcept {
    if (this != &rhs) {
      std::unique_lock lock_rhs(rhs.mtx_);
      std::unique_lock lock_this(mtx_);
      std::lock(lock_rhs, lock_this);
      columns = std::move(rhs.columns);
      name = std::move(rhs.name);
    }

    return *this;
  }

  [[nodiscard]] std::vector<std::uint8_t> into_vec();

  static DatabaseTable from_vec(const std::vector<std::uint8_t> &in,
                                const std::string &name);

  static DatabaseTable from_file(std::string const &path, std::string const &name);

  void to_file(const std::string &path);

  cpp::result<void, std::string>
  try_insert(std::string const &database,
             List<std::variant<Parser::String, Parser::UInt, Parser::Int,
                               Parser::Double, Parser::Bool>> &&values) {

    // First we verify type casting safeness
    if (values.len() != columns.len()) {
      std::stringstream error;
      error << "The number of fields does not match.\n"
            << fmt::format(
                   "Attempt to insert {} elements into a {} element column\n",
                   values.len(), columns.len())
            << "Make sure you follow the positional arguments described by:\n"
            << columns;
      return cpp::fail(error.str());
    }

    using Value =
        std::variant<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
                     std::int8_t, std::int16_t, std::int32_t, std::int64_t,
                     std::string, double, bool>;
    std::vector<Value> to_insert;

    Node<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double,
                      Parser::Bool>> *current = values.first();
    Node<KeyValue<std::string, Layout>> *current_column = columns.first();
    for (int i = 0; i < values.len(); i++) {
      using Variant = std::variant<Parser::String, Parser::UInt, Parser::Int,
                                   Parser::Double, Parser::Bool>;

      Layout &current_layout = current_column->value.value;
      std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double,
                   Parser::Bool> &current_value = current->value;

      // Here is where we do actually verify type matching
      if (std::holds_alternative<Parser::String>(current_value)) {
        if (current_layout.type == ColumnType::str) {
          std::string value = get<Parser::String>(current_value).value;
          if (value.length() > current_layout.size) {
            return cpp::fail(fmt::format(
                "Cannot push string {} bytes long to field {} which takes a {}",
                value.length(), i, current_layout.ly_to_string()));
          } else {
            to_insert.emplace_back(value);
          }
        } else {
          return cpp::fail(
              fmt::format("Cannot push string to field {} which takes a {}", i,
                          current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Double>(current_value)) {
        if (current_layout.type == ColumnType::f64) {
          double value = get<Parser::Double>(current_value).value;
          to_insert.emplace_back(value);
        } else {
          return cpp::fail(
              fmt::format("Cannot push string to field {} which takes a {}", i,
                          current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::UInt>(current_value)) {
        if (current_layout.type == ColumnType::u8 ||
            current_layout.type == ColumnType::u16 ||
            current_layout.type == ColumnType::u32 ||
            current_layout.type == ColumnType::u64 ||
            current_layout.type == ColumnType::i8 ||
            current_layout.type == ColumnType::i16 ||
            current_layout.type == ColumnType::i32 ||
            current_layout.type == ColumnType::i64) {
          switch (current_layout.type) {
          case ColumnType::u8: {
            std::uint8_t value = 0;
            std::uint16_t value_raw;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value_raw;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::uint8_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              value = static_cast<uint8_t>(value_raw);
              std::cout << value << std::endl;
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::u16: {
            std::uint16_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::uint16_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::u32: {
            std::uint32_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::uint32_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::u64: {
            std::uint64_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::uint64_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i8: {
            std::int8_t value = 0;
            std::int16_t value_raw;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value_raw;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::int8_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              value = static_cast<int8_t>(value_raw);
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i16: {
            std::int16_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::int16_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i32: {
            std::int32_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::int32_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i64: {
            std::int64_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::UInt>(current_value).value;
            temp >> value;
            if (temp.fail() || std::get<Parser::UInt>(current_value).value >
                                   std::numeric_limits<std::int64_t>::max()) {
              return cpp::fail(
                  fmt::format("Cannot push uint to field {} which takes a {} "
                              "because the value is too big (overflows)",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          default:
            return cpp::fail(
                fmt::format("Cannot push uint to field {} which takes a {}", i,
                            current_layout.ly_to_string()));
          }
        } else {
          return cpp::fail(
              fmt::format("Cannot push uint to field {} which takes a {}", i,
                          current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Int>(current_value)) {
        if (current_layout.type == ColumnType::i8 ||
            current_layout.type == ColumnType::i16 ||
            current_layout.type == ColumnType::i32 ||
            current_layout.type == ColumnType::i64) {
          switch (current_layout.type) {
          case ColumnType::i8: {
            std::int8_t value = 0;
            std::int16_t value_raw;
            std::stringstream temp;
            temp << std::get<Parser::Int>(current_value).value;
            temp >> value_raw;
            if (temp.fail() ||
                std::get<Parser::Int>(current_value).value >
                    std::numeric_limits<std::int8_t>::max() ||
                std::get<Parser::Int>(current_value).value <
                    std::numeric_limits<std::int8_t>::min()) {
              return cpp::fail(
                  fmt::format("Cannot push int to field {} which takes a {} "
                              "because the value overflows",
                              i, current_layout.ly_to_string()));
            } else {
              value = static_cast<int8_t>(value_raw);
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i16: {
            std::int16_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::Int>(current_value).value;
            temp >> value;
            if (temp.fail() ||
                std::get<Parser::Int>(current_value).value >
                    std::numeric_limits<std::int16_t>::max() ||
                std::get<Parser::Int>(current_value).value <
                    std::numeric_limits<std::int16_t>::min()) {
              return cpp::fail(
                  fmt::format("Cannot push int to field {} which takes a {} "
                              "because the value overflows",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i32: {
            std::int32_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::Int>(current_value).value;
            temp >> value;
            if (temp.fail() ||
                std::get<Parser::Int>(current_value).value >
                    std::numeric_limits<std::int32_t>::max() ||
                std::get<Parser::Int>(current_value).value <
                    std::numeric_limits<std::int32_t>::min()) {
              return cpp::fail(
                  fmt::format("Cannot push int to field {} which takes a {} "
                              "because the value overflows",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          case ColumnType::i64: {
            std::int64_t value = 0;
            std::stringstream temp;
            temp << std::get<Parser::Int>(current_value).value;
            temp >> value;
            if (temp.fail() ||
                std::get<Parser::Int>(current_value).value >
                    std::numeric_limits<std::int64_t>::max() ||
                std::get<Parser::Int>(current_value).value <
                    std::numeric_limits<std::int64_t>::min()) {
              return cpp::fail(
                  fmt::format("Cannot push int to field {} which takes a {} "
                              "because the value overflows",
                              i, current_layout.ly_to_string()));
            } else {
              to_insert.emplace_back(value);
            }
          } break;
          default:
            return cpp::fail(
                fmt::format("Cannot push int to field {} which takes a {}", i,
                            current_layout.ly_to_string()));
          }
        } else {
          return cpp::fail(
              fmt::format("Cannot push int to field {} which takes a {}", i,
                          current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Bool>(current_value)) {
        if (current_layout.type == ColumnType::rbool) {
          to_insert.emplace_back(std::get<Parser::Bool>(current_value).value);
        } else {
          return cpp::fail(
              fmt::format("Cannot push bool to field {} which takes a {}", i,
                          current_layout.ly_to_string()));
        }
      } else
        return cpp::fail(fmt::format("Cannot push to field {}", i));

      current = current->next;
      current_column = current_column->next;
    }

    // Up to this point to_insert is full of values that are the correct type
    // We iterate once more each layout column

    std::unique_lock<std::shared_mutex> lock(mtx_);

    current_column = columns.first();
    for (std::size_t i = 0; i < columns.len(); i++) {
      // Sleep 2000 miliseconds
      FileManager::Path column_path = FileManager::Path("data") / database /
                                      name /
                                      (current_column->value.key + ".col");

      if (!column_path.exists()) {
        return cpp::fail(
            fmt::format("Column {} does not exist", current_column->value.key));
      }

    #define ENCODE(TYPE) if (std::holds_alternative<TYPE>(to_insert[i])) {\
      TYPE value = std::get<TYPE>(to_insert[i]);\
      FileManager::append_to_file(column_path.path,\
                                  Serialized::serialize(value));\
    }

      if (std::holds_alternative<std::string>(to_insert[i])) {
        std::string value = std::get<std::string>(to_insert[i]);
        auto result = columns.get_at(i);
        if (result == nullptr) {
          return cpp::fail(fmt::format(
              "Cannot push string to field. Failed to get string size"));
        }
        auto size = result->value.value.size;
        FileManager::append_to_file(column_path.path,
                                    Serialized::serialize(value, size));
      } else {
        ENCODE(std::uint64_t)
        else ENCODE(std::int64_t)
        else ENCODE(std::uint32_t)
        else ENCODE(std::int32_t)
        else ENCODE(std::uint16_t)
        else ENCODE(std::int16_t)
        else ENCODE(std::uint8_t)
        else ENCODE(std::int8_t)
        else ENCODE(bool)
        else ENCODE(double)
        else return cpp::fail(fmt::format("Cannot push to field {}", i));
      }

      current_column = current_column->next;
    }

    return {};
  }

  bool operator==(DatabaseTable const &other) const;

  DatabaseTable(const std::string &name, KeyValueList<std::string, Layout> &layout)
      : name(fmt::format("{}", name)), columns(std::move(layout)), mtx_() {}

  static cpp::result<DatabaseTable, std::string>
  createTable(std::string database, std::string &table_name,
              KeyValueList<std::string, Layout> &layout);
};
#endif // TABLE_HPP
