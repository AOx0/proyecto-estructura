#include <fstream>
#include <iostream>

#include "fm.hpp"
#include "table.hpp"
#include "fmt/core.h"
#include "linkedList.hpp"
#include "serializer.hpp"

std::vector<std::uint8_t> Table::into_vec() {
  std::vector<std::uint8_t> resultado{};

  rows.for_each_node([&](Node<KeyValue<std::string, Layout>> * entry){
    const char *str = entry->value.key.c_str();
    size_t i = 0;
    
     
    while (str[i] != '\0') {
      resultado.push_back(str[i]);
      i++;
    }
    resultado.push_back(0x00);
    
    DynArray serlialized_layout = serialize_layout(entry->value.value);
    
    for (int i=0; i<serlialized_layout.length; i++) resultado.push_back(serlialized_layout.array[i]);
    
    resultado.push_back(0x00);
    resultado.push_back(0x73);
    resultado.push_back(0x73);      
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

    i+=1;
    
    std::vector<uint8_t> serialized_layout;
    while (!(in[i] == 0 && in[i+1] == 0x73 && in[i+2] == 0x73)) {
      serialized_layout.push_back(in[i]);
      i+=1;
    }
    
    i+=3;
   
    Layout layout = deserialize_layout(&serialized_layout.front(), serialized_layout.size());
    
    rows.insert(name, layout);
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
  
  if (!p.exists()) 
    return cpp::fail(fmt::format("Database {} does not exist", database));

  auto table_folder = p/table_name;
  
  if (table_folder.exists())
    return cpp::fail(fmt::format("Table {} does exist. Aborting table creation", table_name));

  if (!table_folder.create_as_dir()) 
    return cpp::fail(fmt::format("Failed while creating directory {}",table_folder.path));

  auto table_info = table_folder/"info.tbl";
  FileManager::write_to_file(table_info.path, t.into_vec());

  auto node = layout.for_each([&](const KeyValue<std::string, Layout> & value){
      auto column_file = table_folder/(value.key + ".col");
      if (!column_file.create_as_file()) {
        return true;
      }
      return false;
  });
  
  if (node != nullptr) {
    return cpp::fail(fmt::format("Failed while creating column file {}", (table_folder/(node->value.key + ".col")).path));
  }

  return t;
}