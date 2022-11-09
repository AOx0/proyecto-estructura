#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <result.hpp>
#include <vector>

#include "fm.hpp"
#include "table.hpp"

struct DataBase {
  KeyValueList<std::string, std::shared_ptr<DatabaseTable>> tables;
  std::string nombre;
  std::shared_mutex mutex;

  DataBase(std::string name)
      : tables(std::move(KeyValueList<std::string, std::shared_ptr<DatabaseTable>>())),
        nombre(name) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    load_tables();
  }

  // << operator
  friend std::ostream &operator<<(std::ostream &os, const DataBase &db) {
    auto tables = db.get_tables();

    if (tables.has_error()) {
      os << fmt::format("Error {}", tables.error());
      return os;
    }

    os << "{";

    for (auto &table : tables.value()) {
      os << table << ", ";
    }
    os << "}";
    return os;
  }

  void load_tables() {
    auto db_path = FileManager::Path("data") / nombre;

    std::vector<std::string> result;
    auto contents = FileManager::list_dir(db_path.path);

    if (contents.has_error()) {
      return;
    }

    for (const auto &entry : contents.value()) {
      auto info_file = entry / "info.tbl";
      if (entry.exists() && entry.is_dir() && info_file.exists() &&
          info_file.is_file()) {
        tables.insert(
            info_file.get_parent().get_file_name(),
            std::make_shared<DatabaseTable>(DatabaseTable::from_file(
                info_file.path, info_file.get_parent().get_file_name())));
      }
    }

    return;
  }

  cpp::result<std::vector<std::string>, std::string> get_tables() const {
    auto db_path = FileManager::Path("data") / nombre;

    if (!db_path.exists()) {
      return cpp::fail(fmt::format("Database {} does not exist", nombre));
    }

    std::vector<std::string> result;
    auto contents = FileManager::list_dir(db_path.path);

    if (contents.has_error()) {
      return cpp::fail(contents.error());
    }

    for (const FileManager::Path &entry : contents.value()) {
      auto info_file = entry / "info.tbl";
      if (entry.exists() && entry.is_dir() && info_file.exists() &&
          info_file.is_file())
        result.push_back(entry.get_file_name());
    }

    return result;
  }

  // Move constructor
  // Move constructors should be marked with except
  DataBase(DataBase &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mutex);
    tables = rhs.tables;
    nombre = rhs.nombre;
  }

  // Move operator
  DataBase &operator=(DataBase &&rhs) noexcept {
    if (this != &rhs) {
      std::unique_lock lock_rhs(rhs.mutex);
      std::unique_lock lock_this(mutex);
      std::lock(lock_rhs, lock_this);
      tables = rhs.tables;
      nombre = rhs.nombre;
    }

    return *this;
  }

  cpp::result<void, std::string> delete_table_dir(std::string database,
                                                  std::string table);
  // delete table in memory

  bool operator==(const DataBase &other) const;

  static cpp::result<DataBase, std::string> create(const std::string &name);
};

#endif // DATABASE_HPP
