#ifndef FM_HPP
#define FM_HPP

#include <iostream>
#include <fstream>
#include <vector>

namespace FileManager {
  std::vector<uint8_t> read_to_vec(const std::string & path);
  void write_to_file(const std::string & path, const std::vector<uint8_t> & contents);
}

#endif //FM_HPP
