#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <atomic>
#include <memory>
#include <result.hpp>

#include "table.hpp"
#include "fm.hpp"

struct DataBase {
  std::shared_ptr<std::atomic_int32_t> using_db;
  KeyValueList<std::string, std::shared_ptr<Table>> tables; 
  std::string nombre;

    DataBase(std::string name)
    : tables(std::move(KeyValueList<std::string, std::shared_ptr<Table>>()))
    , nombre(name)
    , using_db(std::make_shared<std::atomic_int32_t>(0))
  {}

  // << operator
  friend std::ostream &operator<<(std::ostream &os, const DataBase &db) {
    auto tables = db.get_tables();
    
    if (tables.has_error()) {
      os << fmt::format("Error {}", tables.error());
      return os;
    }
     
    os << "DataBase: (" << db.using_db << "){";
    
    for (auto &table : tables.value()) {
      os << table << ", ";
    }
    os << "}";
    return os;
  }
  
  void load_tables() {
    auto db_path = FileManager::Path("data")/nombre;
    
    std::vector<std::string> result;
    auto contents = FileManager::list_dir(db_path.path);
    
    if (contents.has_error()) {
      return;
    }
    
    for (const auto & entry : contents.value()) {
      auto info_file = entry/"info.tbl";
      if (entry.exists() && entry.is_dir() && 
          info_file.exists() && info_file.is_file()) {
          tables.insert(info_file.get_parent().get_file_name(), std::make_shared<Table>(Table::from_file(info_file.path, info_file.get_parent().get_file_name())));
      }
    }
    
    return;
  }
  
  cpp::result<std::vector<std::string>, std::string> get_tables() const {
    auto db_path = FileManager::Path("data")/nombre;
    
    if (!db_path.exists()) {
      return cpp::fail(fmt::format("Database {} does not exist", nombre));  
    }
    
    std::vector<std::string> result;
    auto contents = FileManager::list_dir(db_path.path);
    
    if (contents.has_error()) {
      return cpp::fail(contents.error());
    }
    
    for (const FileManager::Path & entry : contents.value()) {
      auto info_file = entry/"info.tbl";
      if (entry.exists() && entry.is_dir() && 
          info_file.exists() && info_file.is_file()) 
        result.push_back(entry.get_file_name());
    }
    
    return result;
  }

  // Move constructor
  // Move constructors should be marked with except
  DataBase(DataBase &&rhs) noexcept {
    tables = rhs.tables;
    nombre = rhs.nombre;
    using_db = rhs.using_db;
  }

  // Copy constructor
  DataBase(DataBase const &rhs) {
    tables = rhs.tables;
    nombre = rhs.nombre;
    using_db = rhs.using_db;
  }

  // Move operator
  DataBase &operator=(DataBase &&rhs) noexcept {
    if (this != &rhs) {
      tables = rhs.tables;
      nombre = rhs.nombre;
      using_db = rhs.using_db;
    }

    return *this;
  }

  void delete_table_dir (std::string database, std::string table);
  //delete table in memory



  bool operator==(const DataBase &other) const;

  static cpp::result<DataBase, std::string> create(const std::string &name);
};

#endif // DATABASE_HPP
