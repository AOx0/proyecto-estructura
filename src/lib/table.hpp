#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <map>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <result.hpp>
#include <optional>

#include "linkedList.hpp"

enum Type {
  u8 = 1,
  u16 = 2,
  u64 = 3,
  i8 = 4,
  i16 = 5,
  i64 = 6,
  f32 = 7,
  f64 = 8,
  str = 9
};

struct Layout {
  uint8_t size;
  bool optional;
  Type type;

  bool operator==(Layout const &other) const;
};

struct TableInstance {
  KeyValueList<std::string, std::tuple<Layout,void*>> data;

  /*TableInstance(std::string table_name) : data(KeyValueList<std::string, std::tuple<Layout,void*>>()) {}

  };*/
};

struct Table {
  std::optional<TableInstance> instance;
  KeyValueList<std::string, Layout> rows;
  std::shared_mutex mtx_;

  // Move constructor
  // Move constructors should be marked with except
  Table(Table &&rhs) noexcept {
    std::unique_lock<std::shared_mutex> lock(rhs.mtx_);
    rows = std::move(rhs.rows);
  }

  // Move operator
  Table &operator=(Table &&rhs) noexcept {
    if (this != &rhs) {
      std::unique_lock lock_rhs(rhs.mtx_);
      std::unique_lock lock_this(mtx_);
      std::lock(lock_rhs, lock_this);
      rows = std::move(rhs.rows);
    }

    return *this;
  }

  [[nodiscard]] std::vector<std::uint8_t> into_vec();

  static Table from_vec(const std::vector<std::uint8_t> &in);

  static Table from_file(std::string const &path);

  void to_file(const std::string &path);

  bool operator==(Table const &other) const;

  Table(KeyValueList<std::string, Layout> &layout) : rows(std::move(layout)), mtx_() {}

  static Table createTable(std::string database, std::string &name, KeyValueList<std::string, Layout> &layout, std::string &path);
};

#endif // TABLE_HPP
