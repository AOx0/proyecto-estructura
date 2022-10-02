#include "database.hpp"
#include <vector>

#include "fm.hpp"

std::vector<std::uint8_t> DataBase::into_vec() const {
  std::vector<std::uint8_t> resultado{};

  for (auto &table : tables) {
    const char *str = table.c_str();
    size_t i = 0;
    while (str[i] != '\0') {
      resultado.push_back(str[i]);
      i++;
    }
    resultado.push_back('\0');
  }

  resultado.push_back('\0');
  return resultado;
}

DataBase DataBase::from_vec(const std::vector<std::uint8_t> &in) {
  DataBase result{.tables = {}};
  size_t i = 0;

  while (in[i] != '\0') {
    std::string name;
    while (in[i] != '\0') {
      name += in[i];
      i++;
    }
    i++;
    result.tables.push_back(name);
  }

  return result;
}

bool DataBase::operator==(const DataBase &other) const {
  return tables == other.tables;
}

DataBase DataBase::from_file(const std::string &path) {
  return DataBase::from_vec(FileManager::read_to_vec(path));
}

void DataBase::to_file(const std::string &path) const {
  FileManager::write_to_file(path, this->into_vec());
}

DataBase DataBase::create(const std::string &path, const std::string &name) {
  DataBase d{};

  FileManager::write_to_file(path + name + ".db", d.into_vec());

  return d;
}
