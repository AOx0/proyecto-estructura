#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <iostream>
#include <vector>

#include "table.hpp"

struct DataBase {
  std::vector<std::string> tables;

  std::vector<std::uint8_t> into_vec() const;
};



#endif // DATABASE_HPP
