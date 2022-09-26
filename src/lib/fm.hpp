#ifndef FM_HPP
#define FM_HPP

#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>

namespace FileManager {
std::vector<uint8_t> read_to_vec(const std::string &path);
void write_to_file(const std::string &path,
                   const std::vector<uint8_t> &contents);
} // namespace FileManager

#endif // FM_HPP
