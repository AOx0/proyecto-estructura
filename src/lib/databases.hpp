#ifndef DATABASES_HPP
#define DATABASES_HPP

#include <iostream>
#include <fmt/format.h>
#include <result.hpp>

#include "fm.hpp"
#include "database.hpp"

struct Databases {
  std::mutex mutex;
  KeyValueList<std::string, DataBase> dbs;

  cpp::result<void, std::string> load() {
    auto current_path = FileManager::Path::get_working_dir();
    auto data_path = current_path / "data";

    if (!data_path.exists()) {
      return cpp::fail(
          fmt::format("Data folder does not exist at {}", data_path.path));
    }

    auto contents = FileManager::list_dir("data/");

    if (contents.has_error())
      return cpp::fail(contents.error());

    for (auto &entry : contents.value()) {
      if (entry.is_dir()) {
        auto db = DataBase(entry.get_file_name());
        db.load_tables();
        dbs.insert(entry.get_file_name(), db);
      }
    }

    return {};
  }

  void add(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    dbs.insert(name, DataBase({}));
  }

  void remove(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    dbs.delete_key(name);
  }

  // To string
  std::string to_string() {
    std::lock_guard<std::mutex> lock(mutex);
    std::stringstream ss;
    ss << "Databases: " << dbs << "\n";
    return ss.str();
  }
  std::vector<std::string> get_db_names() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::string> names;
    dbs.for_each([&](const KeyValue<std::string, DataBase> &keyValue) {
      names.push_back(keyValue.key);
      return false;
    });
    return names;
  }

  cpp::result<void, std::string> delete_table(std::string table,
                                              std::string name) {
    std::lock_guard<std::mutex> lock(mutex);
    auto db =
        dbs.get(name); // método get me da el apuntador de lo que estas buscando
    if (db != nullptr) {

      auto table_ptr = db->tables.get(table);

      if (table_ptr == nullptr) {
        return cpp::fail(
            fmt::format("DatabaseTable {}.{} does not exist", name, table));
      } else {
        auto result = db->delete_table_dir(name, table);

        if (result.has_error())
          return cpp::fail(result.error());

        db->tables.delete_key(table);
      }
    } else {
      return cpp::fail(fmt::format("Database {} does not exist.", name));
    }

    return {};
  }

  cpp::result<void, std::string> delete_database(std::string name) {
    std::lock_guard<std::mutex> lock(mutex);

    auto result = dbs.get(name);

    if (result == nullptr) {
      return cpp::fail(fmt::format("Database {} does not exist.", name));
    } else {
      auto db_path = FileManager::Path("data") / name;
      if (!db_path.remove()) {
        return cpp::fail(fmt::format(
            "Something went wrong while deleting folder {}", db_path.path));
      }
      dbs.delete_key(name);
    }

    return {};
  }
};

#endif //DATABASES_HPP
