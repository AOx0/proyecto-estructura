#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <map>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <result.hpp>
#include <optional>
#include <variant>

#include "analyzer/parser.hpp"
#include "linkedList.hpp"
#include "serializer.hpp"
#include "fm.hpp"


struct Table {
  KeyValueList<std::string, Layout> columns;
  std::shared_mutex mtx_;
  std::string name;
  
  friend std::ostream &operator<<(std::ostream &os, Table const &table) {
    os << "Table " << table.name << ":\n";
    
    table.columns.for_each_c([&](const KeyValue<std::string, Layout> & keyval){
      os << "    " << keyval.key << " : " << keyval.value << '\n';
      return false;
    });
    
    return os;
  }

  // Move constructor
  // Move constructors should be marked with except
  Table(Table &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mtx_);
    columns = std::move(rhs.columns);
    name = std::move(rhs.name);
  }

  // Move operator
  Table &operator=(Table &&rhs) noexcept {
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

  static Table from_vec(const std::vector<std::uint8_t> &in, const std::string & name);

  static Table from_file(std::string const &path, std::string const &name);

  void to_file(const std::string &path);
  
  cpp::result<void, std::string> try_insert(
      std::string const & database,
      List<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double, Parser::Bool>> &&values
  ) {
    // First we verify type casting safeness
    if (values.len() != columns.len()) {
      std::stringstream error;
      error
          << "The number of fields does not match.\n"
          << fmt::format("Attempt to insert {} elements into a {} element column\n", values.len(), columns.len())
          << "Make sure you follow the positional arguments described by:\n"
          << columns;
      return cpp::fail(error.str());
    }

    using Value = std::variant<
        std::uint8_t, std::uint16_t, std::uint32_t,
        std::uint64_t, std::int8_t, std::int16_t,
        std::int32_t, std::int64_t, std::string, double, bool>;
    std::vector<Value> to_insert;

    Node<std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double, Parser::Bool>> *current = values.first();
    Node<KeyValue<std::string, Layout>> *current_column = columns.first();
    for (int i = 0; i < values.len(); i++) {
      using Variant = std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double, Parser::Bool>;

      Layout &current_layout = current_column->value.value;
      std::variant<Parser::String, Parser::UInt, Parser::Int, Parser::Double, Parser::Bool> &current_value = current->value;

      // Here is where we do actually verify type matching
      if (std::holds_alternative<Parser::String>(current_value)) {
        if (current_layout.type == ColumnType::str) {
          std::string value = get < Parser::String > (current_value).value;
          if (value.length() > current_layout.size) {
            return cpp::fail(
                fmt::format(
                    "Cannot push string {} bytes long to field {} which takes a {}",
                    value.length(), i, current_layout.ly_to_string()));
          } else {
            to_insert.emplace_back(value);
          }
        } else {
          return cpp::fail(
              fmt::format(
                  "Cannot push string to field {} which takes a {}",
                  i, current_layout.ly_to_string()));
        }
      } else if (std::holds_alternative<Parser::Double>(current_value)) {
        if (current_layout.type == ColumnType::f64) {
          double value = get < Parser::Double > (current_value).value;
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
            case ColumnType::u8: {
              std::uint8_t value = 0;
              std::uint16_t value_raw;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value_raw;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint8_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                value = static_cast<uint8_t>(value_raw);
                std::cout << value << std::endl;
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::u16: {
              std::uint16_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint16_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::u32: {
              std::uint32_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint32_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::u64: {
              std::uint64_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::uint64_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i8: {
              std::int8_t value = 0;
              std::int16_t value_raw;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value_raw;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int8_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                value = static_cast<int8_t>(value_raw);
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i16: {
              std::int16_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int16_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i32: {
              std::int32_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int32_t>::max()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push uint to field {} which takes a {} because the value is too big (overflows)",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i64: {
              std::int64_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::UInt>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::UInt>(current_value).value > std::numeric_limits<std::int64_t>::max()) {
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
            case ColumnType::i8: {
              std::int8_t value = 0;
              std::int16_t value_raw;
              std::stringstream temp;
              temp << std::get<Parser::Int>(current_value).value;
              temp >> value_raw;
              if (temp.fail() || std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int8_t>::max() ||
                  std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int8_t>::min()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push int to field {} which takes a {} because the value overflows",
                        i, current_layout.ly_to_string()));
              } else {
                value = static_cast<int8_t>(value_raw);
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i16: {
              std::int16_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::Int>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int16_t>::max() ||
                  std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int16_t>::min()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push int to field {} which takes a {} because the value overflows",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i32: {
              std::int32_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::Int>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int32_t>::max() ||
                  std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int32_t>::min()) {
                return cpp::fail(
                    fmt::format(
                        "Cannot push int to field {} which takes a {} because the value overflows",
                        i, current_layout.ly_to_string()));
              } else {
                to_insert.emplace_back(value);
              }
            }
              break;
            case ColumnType::i64: {
              std::int64_t value = 0;
              std::stringstream temp;
              temp << std::get<Parser::Int>(current_value).value;
              temp >> value;
              if (temp.fail() ||
                  std::get<Parser::Int>(current_value).value > std::numeric_limits<std::int64_t>::max() ||
                  std::get<Parser::Int>(current_value).value < std::numeric_limits<std::int64_t>::min()) {
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
      } else if (std::holds_alternative<Parser::Bool>(current_value)) {
        if (current_layout.type == ColumnType::rbool) {
          to_insert.emplace_back(std::get<Parser::Bool>(current_value).value);
        } else {
          return cpp::fail(
              fmt::format(
                  "Cannot push bool to field {} which takes a {}",
                  i, current_layout.ly_to_string()));
        }
      } else
        return cpp::fail(fmt::format("Cannot push to field {}", i));

      current = current->next;
      current_column = current_column->next;
    }

    // Up to this point to_insert is full of values that are the correct type
    // We iterate once more each layout column

    current_column = columns.first();
    for (std::size_t i = 0; i < columns.len(); i++) {
      FileManager::Path column_path = FileManager::Path("data") / database / name / (current_column->value.key + ".col");

      if (!column_path.exists()) {
        return cpp::fail(fmt::format("Column {} does not exist", current_column->value.key));
      }

      if (std::holds_alternative<std::uint64_t>(to_insert[i])) {
        std::uint64_t value = std::get<std::uint64_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::int64_t>(to_insert[i])) {
        std::int64_t value = std::get<std::int64_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::string>(to_insert[i])) {
        std::string value = std::get<std::string>(to_insert[i]);
        auto result = columns.get_at(i);
        if (result == nullptr) {
          return cpp::fail(fmt::format("Cannot push string to field. Failed to get string size"));
        }
        auto size = result->value.value.size;
        FileManager::append_to_file(column_path.path, Serialized::serialize(value, size));
      } else if (std::holds_alternative<bool>(to_insert[i])) {
        bool value = std::get<bool>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::uint8_t>(to_insert[i])) {
        std::uint8_t value = std::get<std::uint8_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::uint16_t>(to_insert[i])) {
        std::uint16_t value = std::get<std::uint16_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::uint32_t>(to_insert[i])) {
        std::uint32_t value = std::get<std::uint32_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::int8_t>(to_insert[i])) {
        std::int8_t value = std::get<std::int8_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::int16_t>(to_insert[i])) {
        std::int16_t value = std::get<std::int16_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<std::int32_t>(to_insert[i])) {
        std::int32_t value = std::get<std::int32_t>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else if (std::holds_alternative<double>(to_insert[i])) {
        double value = std::get<double>(to_insert[i]);
        FileManager::append_to_file(column_path.path, Serialized::serialize(value));
      } else {
        return cpp::fail(fmt::format("Cannot push to field {}", i));
      }

      current_column = current_column->next;
    }
    
    return {};
  }

  bool operator==(Table const &other) const;

  Table(const std::string & name, KeyValueList<std::string, Layout> &layout)
    : name(fmt::format("{}", name))
    , columns(std::move(layout))
    , mtx_()
  {}

  static cpp::result<Table, std::string> createTable(
      std::string database, 
      std::string &table_name, 
      KeyValueList<std::string, Layout> &layout
    );
};


struct ColumnInstance {
  KeyValueList<Parser::NameAndSub, std::optional<void *>> data;
  
  static cpp::result<ColumnInstance, std::string> load_column(std::string database, std::string table, std::string column, Table & descriptor) {
    std::unique_lock<std::shared_mutex> lock(descriptor.mtx_);
    
    Node<KeyValue<std::string, Layout>> * node = descriptor.columns.for_each([&](const auto & keyvalue){
      if (keyvalue.key == column) {
        return true;
      }
      return false;
    });
    
    if (node == nullptr) {
      return cpp::fail(fmt::format("Column {} does not exist in {}.{}", column, database, table));
    }
    
    FileManager::Path column_file = FileManager::Path("data")/database/table/(column + ".col");
    
    if (!column_file.exists()) {
      return cpp::fail(fmt::format("Failed to get path to column {}, path {} does not exist", column, column_file.path));
    } else if (!column_file.is_file()) {
      return cpp::fail(fmt::format("The column path {} is not a file.", column_file.path));
    }
    
    
    if (node->value.value.type == ColumnType::u8) {
      std::vector<uint8_t> result; 
      auto contents = FileManager::read_to_vec(column_file.path);
      
      for (size_t i=0; i<contents.size(); i++) {
        result.push_back(Serialized::du8(&contents[i], 1));
      }
      
      for (auto & value: result)
        std::cout << static_cast<uint8_t>(value) << " ";
      std::cout << "\n";
    } 
    
    return cpp::fail(fmt::format("Failed to load contents for column {} from {}.{}", column, database, table));
  }
  
  /*TableInstance(std::string table_name) : data(KeyValueList<std::string, std::tuple<Layout,void*>>()) {}
  };*/
};

#endif // TABLE_HPP
