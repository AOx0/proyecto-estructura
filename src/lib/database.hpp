#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <iostream>
#include <vector>

#include "table.hpp"

struct DataBase {
  std::vector<std::string> tables;

  std::vector<std::uint8_t> into_vec() const;

  static DataBase from_vec(const std::vector<std::uint8_t> &in);

  static DataBase from_file(const std::string &path);

  void to_file(const std::string &path) const;

  bool operator==(const DataBase &other) const;
};

#endif // DATABASE_HPP
