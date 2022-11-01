#include <fstream>
#include <iostream>

#include "fm.hpp"
#include "table.hpp"
#include "fmt/core.h"
#include "linkedList.hpp"

std::vector<std::uint8_t> Table::into_vec() {
  std::vector<std::uint8_t> resultado{};

  rows.for_each_node([&](Node<KeyValue<std::string, Layout>> * entry){
    const char *str = entry->value.key.c_str();
    size_t i = 0;
    while (str[i] != '\0') {
      resultado.push_back(str[i]);
      i++;
    }
    resultado.push_back('\0');
    resultado.push_back(entry->value.value.type);
    resultado.push_back(entry->value.value.size);
    resultado.push_back(entry->value.value.optional);
    return false;
  });

  resultado.push_back('\0');
  return resultado;
}

Table Table::from_vec(const std::vector<std::uint8_t> &in) {
  KeyValueList<std::string, Layout> rows;

  size_t i = 0;
  while (in[i] != '\0') {
    std::string name;
    while (in[i] != '\0') {
      name += in[i];
      i++;
    }
    Type type((Type) in[i + 1]);
    uint8_t size(in[i + 2]);
    bool optional(in[i + 3]);
    rows.insert(name, {.size = size, .optional = optional, .type = type});
    i += 4;
  }

  return rows;
}

bool Table::operator==(const Table &other) const { return rows == other.rows; }

Table Table::from_file(std::string const &path) {
  return Table::from_vec(FileManager::read_to_vec(path));
}

void Table::to_file(const std::string &path) {
  FileManager::write_to_file(path, this->into_vec());
}

bool Layout::operator==(const Layout &other) const {
  int result = 0;
  result += size == other.size;
  result += optional == other.optional;
  result += type == other.type;

  return result == 3;
}

cpp::result<Table, std::string> Table::createTable(std::string database, std::string &table_name, KeyValueList<std::string, Layout> &layout) {
  Table t(layout);
  
  auto p = FileManager::Path("data")/database;
  
  if (!p.exists()) return cpp::fail(fmt::format("Database {} does not exist", database));
  
  auto pdata = p + fmt::format("{}.tbl", table_name);
  p+=fmt::format("{}.tbls", table_name);

  auto result = pdata.create_as_file();
  
  if (!result) {
    return cpp::fail(fmt::format("Failed creating data file {}", pdata.path));
  } 
  
  FileManager::write_to_file(p.path, t.into_vec());
  return t;
}