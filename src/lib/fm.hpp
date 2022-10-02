#ifndef FM_HPP
#define FM_HPP

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace FileManager {
std::vector<uint8_t> read_to_vec(const std::string &path);
void write_to_file(const std::string &path,
                   const std::vector<uint8_t> &contents);


  namespace Path {
    extern "C" bool exists(char *);
    extern "C" bool is_dir(char *);
    extern "C" bool is_file(char *);
    extern "C" bool is_symlink(char *);
    extern "C" bool is_absolute(char *);
    extern "C" bool is_relative(char *);
    extern "C" char *get_absolute(char *);
    extern "C" char *get_parent(char *);
    extern "C" char *get_file_name(char *);
    extern "C" char *get_extension(char *);
    extern "C" char *get_user_home();
    extern "C" char *get_working_dir();
    extern "C" bool *set_working_dir(char *);
  }
} // namespace FileManager

#endif // FM_HPP
