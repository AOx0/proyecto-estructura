#ifndef DATABASES_HPP
#define DATABASES_HPP

#include <iostream>
#include <fmt/format.h>
#include <result.hpp>

#include "fm.hpp"
#include "database.hpp"

struct Databases {
  std::mutex mutex;
  KeyValueList<std::string, shared_ptr<DataBase>> dbs;

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
        dbs.insert(entry.get_file_name(), make_shared<DataBase>(DataBase(entry.get_file_name())));
      }
    }

    return {};
  }

  void add(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex);
    dbs.insert(name, make_shared<DataBase>(DataBase({})));
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
    dbs.for_each([&](const KeyValue<std::string, shared_ptr<DataBase>> &keyValue) {
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
      std::unique_lock<std::shared_mutex> lock(db->get()->mutex);
      auto table_ptr = (*db)->tables.get(table);

      if (table_ptr == nullptr) {
        return cpp::fail(
            fmt::format("DatabaseTable {}.{} does not exist", name, table));
      } else {
        {
          std::unique_lock<std::shared_mutex> lock(table_ptr->get()->mtx_);
          auto result = (*db)->delete_table_dir(name, table);

          if (result.has_error())
            return cpp::fail(result.error());
        }
        (*db)->tables.delete_key(table);
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
      {
        std::unique_lock<std::shared_mutex> lock(result->get()->mutex);

        // Store table locks in a vector
        std::vector<std::unique_lock<std::shared_mutex>> table_locks;
        // Lock all tables in the database
        result->get()->tables.for_each([&](const KeyValue<std::string, shared_ptr<DatabaseTable>> &keyValue) {
          table_locks.emplace_back(std::unique_lock<std::shared_mutex>(keyValue.value->mtx_));
          return false;
        });

        auto db_path = FileManager::Path("data") / name;
        if (!db_path.remove()) {
          return cpp::fail(fmt::format(
              "Something went wrong while deleting folder {}", db_path.path));
        }
      }
      dbs.delete_key(name);
    }

    return {};
  }
};

#endif //DATABASES_HPP
