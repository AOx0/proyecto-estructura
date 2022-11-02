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
  std::string nombre;

    DataBase(std::string name)
    : nombre(name)
    , using_db(std::shared_ptr<std::atomic_int32_t>{0})
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
  
  cpp::result<std::vector<std::string>, std::string> get_tables() const {
    auto db_path = FileManager::Path("data")/nombre;
    
    if (!db_path.exists()) {
      return cpp::fail(fmt::format("Database {} does not exist", nombre));  
    }
    
    std::vector<std::string> result, contents = FileManager::list_dir(db_path.path);
    
    for (const auto & entry : contents) {
      auto path = FileManager::Path(entry);
      auto info_file = path/"info.tbl";
      if (path.exists() && path.is_dir() && 
          info_file.exists() && info_file.is_file()) 
        result.push_back(entry);
    }
    
    return result;
  }

  // Move constructor
  // Move constructors should be marked with except
  DataBase(DataBase &&rhs) noexcept {
    nombre = rhs.nombre;
    using_db = rhs.using_db;
  }

  // Copy constructor
  DataBase(DataBase const &rhs) {
    nombre = rhs.nombre;
    using_db = rhs.using_db;
  }

  // Move operator
  DataBase &operator=(DataBase &&rhs) noexcept {
    if (this != &rhs) {
      nombre = rhs.nombre;
      using_db = rhs.using_db;
    }

    return *this;
  }

  bool operator==(const DataBase &other) const;

  static cpp::result<DataBase, std::string> create(const std::string &name);
};

#endif // DATABASE_HPP
