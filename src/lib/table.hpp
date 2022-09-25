#ifndef TABLE_HPP
#define TABLE_HPP

#include <iostream>
#include <map>
#include <vector>

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

  bool operator==(Layout const & other) const {
    int result = 0;
    result += size == other.size;
    result += optional == other.optional;
    result += type == other.type;

    return result;
  }
};

struct Table {
  std::map<std::string, Layout> rows;

  std::vector<std::uint8_t> into_vec() const;

  static Table from_vec(std::vector<std::uint8_t> const & in);

  bool operator==(Table const & other) const;
};

#endif //TABLE_HPP