#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <atomic>

#include "table.hpp"

struct DataBase {
  std::shared_ptr<std::atomic_int32_t> using_db = 0;
  std::vector<std::string> tables;

  DataBase (std::vector<std::string> tables) : tables(std::move(tables)), using_db(0) {}

  // Move constructor
  // Move constructors should be marked with except
  DataBase ( DataBase && rhs ) noexcept {
    tables = std::move(rhs.tables);
    using_db = rhs.using_db;
  }

  // Move operator
  DataBase & operator= (DataBase && rhs) noexcept {
    if (this != &rhs) {
      tables = std::move(rhs.tables);
      using_db = rhs.using_db;
    }

    return *this;
  }

  [[nodiscard]] std::vector<std::uint8_t> into_vec() const;

  static DataBase from_vec(const std::vector<std::uint8_t> &in);

  static DataBase from_file(const std::string &path);

  void to_file(const std::string &path) const;

  bool operator==(const DataBase &other) const;

  static DataBase create(const std::string &path, const std::string & name);
};

#endif // DATABASE_HPP
