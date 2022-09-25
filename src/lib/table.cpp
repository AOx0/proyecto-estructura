#include "table.hpp"

std::vector<std::uint8_t> Table::into_vec() const {
  std::vector<std::uint8_t> resultado {};

  for (auto & entry: rows) {
    const char * str = entry.first.c_str();
    size_t i = 0;
    while (str[i] != '\0') {
      resultado.push_back(str[i]);
      i++;
    }
    resultado.push_back('\0');
    resultado.push_back(entry.second.type);
    resultado.push_back(entry.second.size);
    resultado.push_back(entry.second.optional);
  }

  resultado.push_back('\0');
  return resultado;
}

Table Table::from_vec(const std::vector<std::uint8_t> &in) {
  Table t { .rows = { } };

  size_t i = 0;
  while (in[i] != '\0') {
    std::string name;
    while (in[i] != '\0') {
      name += in[i];
      i++;
    }
    Type type((Type) in[i + 1]);
    uint8_t  size(in[i+2]);
    bool optional(in[i+3]);
    t.rows[name] = { .size = size, .optional = optional, .type = type  };
    i+=4;
  }

  return t;
}

bool Table::operator==(const Table &other) const {
  return rows == other.rows;
}

bool Layout::operator==(const Layout &other) const {
  int result = 0;
  result += size == other.size;
  result += optional == other.optional;
  result += type == other.type;

  return result;
}
