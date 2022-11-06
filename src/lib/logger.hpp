#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fmt/color.h>
#include <fmt/core.h>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>

#include "fm.hpp"

enum LOG_TYPE_ { INFO, WARN, ERROR, NONE };

struct Logger {
  using Path = FileManager::Path;

  Path path_;
  std::mutex mtx_;

  static std::string show(LOG_TYPE_ type, const std::string &msg);

  static std::string show_ln(LOG_TYPE_ type, const std::string &msg);

private:
  // static method to convert a string to a vec of uint_8
  static std::vector<std::uint8_t> to_vec(const std::string &str);

public:
  explicit Logger(Path path);

  // Static method to log to path_.path and to print to stdout with fmt library
  void log(const std::string &msg);

  // Same method but for warnings
  void warn(const std::string &msg);

  // Same method but for errors
  void error(const std::string &msg);
};

#endif // LOGGER_HPP
