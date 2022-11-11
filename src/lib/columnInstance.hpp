#ifndef COLUMNINSTANCE_HPP
#define COLUMNINSTANCE_HPP

#include "analyzer.hpp"

struct ColumnInstance {
  Parser::NameAndSub column;
  void * data;
  Layout layout;
  std::size_t size;

  ColumnInstance(Parser::NameAndSub column, void * data, Layout layout, std::size_t size) : column(column), data(data), layout(layout), size(size) {}

  static cpp::result<std::variant<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
      std::int8_t, std::int16_t, std::int32_t, std::int64_t,
      std::string, double, bool>, std::string> resolve_value(std::variant<Parser::String, Parser::UInt, Parser::Int,
      Parser::Double, Parser::Bool> current_value, Layout current_layout) {
    // Here is where we do actually verify type matching
    if (std::holds_alternative<Parser::String>(current_value)) {
      if (current_layout.type == ColumnType::str) {
        std::string value = get<Parser::String>(current_value).value;
        if (value.length() > current_layout.size) {
          return cpp::fail(fmt::format(
              "Cannot cast string {} bytes long to field which takes a {}",
              value.length(), current_layout.ly_to_string()));
        } else {
          return value;
        }
      } else {
        return cpp::fail(
            fmt::format("Cannot cast string to field which takes a {}",
                        current_layout.ly_to_string()));
      }
    } else if (std::holds_alternative<Parser::Double>(current_value)) {
      if (current_layout.type == ColumnType::f64) {
        double value = get<Parser::Double>(current_value).value;
        return value;
      } else {
        return cpp::fail(
            fmt::format("Cannot cast string to field which takes a {}",
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)", current_layout.ly_to_string()));
            } else {
              value = static_cast<uint8_t>(value_raw);
              std::cout << value << std::endl;
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              value = static_cast<int8_t>(value_raw);
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast uint to field which takes a {} "
                              "because the value is too big (overflows)",
                              current_layout.ly_to_string()));
            } else {
              return value;
            }
          } break;
          default:
            return cpp::fail(
                fmt::format("Cannot cast uint to field which takes a {}",
                            current_layout.ly_to_string()));
        }
      } else {
        return cpp::fail(
            fmt::format("Cannot cast uint to field which takes a {}",
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
                  fmt::format("Cannot cast int to field which takes a {} "
                              "because the value overflows",
                              current_layout.ly_to_string()));
            } else {
              value = static_cast<int8_t>(value_raw);
              return value;
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
                  fmt::format("Cannot cast int to field which takes a {} "
                              "because the value overflows",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast int to field which takes a {} "
                              "because the value overflows",
                              current_layout.ly_to_string()));
            } else {
              return value;
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
                  fmt::format("Cannot cast int to field which takes a {} "
                              "because the value overflows",
                              current_layout.ly_to_string()));
            } else {
              return value;
            }
          } break;
          default:
            return cpp::fail(
                fmt::format("Cannot cast int to field which takes a {}",
                            current_layout.ly_to_string()));
        }
      } else {
        return cpp::fail(
            fmt::format("Cannot cast int to field which takes a {}",
                        current_layout.ly_to_string()));
      }
    } else if (std::holds_alternative<Parser::Bool>(current_value)) {
      if (current_layout.type == ColumnType::rbool) {
        return std::get<Parser::Bool>(current_value).value;
      } else {
        return cpp::fail(
            fmt::format("Cannot cast bool to field which takes a {}",
                        current_layout.ly_to_string()));
      }
    } else
      return cpp::fail(fmt::format("Cannot cast to field"));
  }

  std::vector<std::string> to_string_vec() {
#define PUSH(TYPE, CAST) if (layout.type == TYPE) {\
      auto * data_cast = reinterpret_cast<std::vector<CAST> *>(data);\
      std::vector<std::string> result;\
      for (std::size_t i = 0; i < size; i++) {\
        result.push_back(fmt::format("{}", +data_cast->at(i)));\
      }                                            \
      return result;                               \
    }

    PUSH(ColumnType::u8, std::uint8_t)
    else PUSH(ColumnType::u16, std::uint16_t)
    else PUSH(ColumnType::u32, std::uint32_t)
    else PUSH(ColumnType::u64, std::uint64_t)
    else PUSH(ColumnType::i8, std::int8_t)
    else PUSH(ColumnType::i16, std::int16_t)
    else PUSH(ColumnType::i32, std::int32_t)
    else PUSH(ColumnType::i64, std::int64_t)
    else PUSH(ColumnType::f64, double)
    else if (layout.type == ColumnType::str) {
      auto *data_cast = reinterpret_cast<std::vector<std::string> *>(data);
      std::vector<std::string> result;
      for (std::size_t i = 0; i < size; i++) {
        result.push_back(fmt::format("{}", data_cast->at(i)));
      }
      return result;
    }
    else if (layout.type == ColumnType::rbool) {
      auto *data_cast = reinterpret_cast<std::vector<bool> *>(data);
      std::vector<std::string> result;
      for (std::size_t i = 0; i < size; i++) {
        result.push_back(fmt::format("{}", data_cast->at(i)));
      }
      return result;
    }
    else
      throw std::runtime_error("Cannot convert to string vec");

  }

  static cpp::result<ColumnInstance, std::string>
  load_column(std::string database, std::string table, std::string column,
              DatabaseTable &descriptor) {
    std::unique_lock<std::shared_mutex> lock(descriptor.mtx_);

    Node<KeyValue<std::string, Layout>> *node =
        descriptor.columns.for_each([&](const auto &keyvalue) {
          if (keyvalue.key == column) {
            return true;
          }
          return false;
        });

    if (node == nullptr) {
      return cpp::fail(fmt::format("Column {} does not exist in {}.{}", column,
                                   database, table));
    }

    FileManager::Path column_file =
        FileManager::Path("data") / database / table / (column + ".col");

    if (!column_file.exists()) {
      return cpp::fail(
          fmt::format("Failed to get path to column {}, path {} does not exist",
                      column, column_file.path));
    } else if (!column_file.is_file()) {
      return cpp::fail(
          fmt::format("The column path {} is not a file.", column_file.path));
    }

    void *data_array = nullptr;
    std::size_t size = 0;

#define DECODE(TYPE, CAST, DECODER) if (node->value.value.type == ColumnType::TYPE) {\
    auto * result = new std::vector<CAST>();\
    auto contents = FileManager::read_to_vec(column_file.path);                      \
     if (contents.size() % node->value.value.size != 0) {\
        return cpp::fail(fmt::format("The column {} is not a multiple of the size of the column type.", column));\
    } else if (contents.empty()) {\
        return cpp::fail(fmt::format("The column {} is empty.", column));\
    }                                                                            \
    for (size_t i = 0; i < contents.size(); i+=node->value.value.size) {\
      result->push_back(Serialized::DECODER(&contents[i], node->value.value.size));\
    }\
    size = result->size();                                                              \
    data_array = result;\
  }

    DECODE(u8, uint8_t, du8)
    else DECODE(u16, uint16_t, du16)
    else DECODE(u32, uint32_t, du32)
    else DECODE(u64, uint64_t, du64)
    else DECODE(i8, int8_t, di8)
    else DECODE(i16, int16_t, di16)
    else DECODE(i32, int32_t, di32)
    else DECODE(i64, int64_t, di64)
    else DECODE(f64, double, df64)
    else if (node->value.value.type == ColumnType::rbool) {
      auto * result = new std::vector<bool>();
      auto contents = FileManager::read_to_vec(column_file.path);

      if (contents.size() % node->value.value.size != 0) {
        return cpp::fail(fmt::format("The column {} is not a multiple of the size of the column type.", column));
      } else if (contents.empty()) {
        return cpp::fail(fmt::format("The column {} is empty.", column));
      }

      for (size_t i = 0; i < contents.size(); i+=node->value.value.size) {
        result->push_back(Serialized::dbool(&contents[i], node->value.value.size));
      }
      size = result->size();
      data_array = result;
    } else if (node->value.value.type == ColumnType::str) {
      auto * result = new std::vector<std::string>();
      auto contents = FileManager::read_to_vec(column_file.path);

      if (contents.size() % (node->value.value.size + 8) != 0) {
        return cpp::fail(fmt::format("The column {} is not a multiple of the size of the column type.", column));
      } else if (contents.empty()) {
        return cpp::fail(fmt::format("The column {} is empty.", column));
      }

      for (size_t i = 0; i < contents.size(); i += node->value.value.size + 8) {
        result->push_back(
            Serialized::dstr(&contents[i], node->value.value.size + 8)
                .to_string());
      }
      size = result->size();
      data_array = result;
    }

    if (data_array == nullptr)
      return cpp::fail(
          fmt::format("Failed to load contents for column {} from {}.{}",
                      column, database, table));
    else
      return ColumnInstance{Parser::NameAndSub{table, column}, data_array,
                            node->value.value, size};
  }
};


#endif //COLUMNINSTANCE_HPP
