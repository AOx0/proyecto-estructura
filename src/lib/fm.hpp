#ifndef FM_HPP
#define FM_HPP

#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <result.hpp>
#include <string>
#include <utility>
#include <vector>

#include "serializer.hpp"

namespace rfm_ {
extern "C" bool exists(char *);
extern "C" bool is_dir(char *);
extern "C" bool is_file(char *);
extern "C" bool is_symlink(char *);
extern "C" bool is_absolute(char *);
extern "C" bool is_relative(char *);
extern "C" bool remove_dir(char *);
extern "C" bool remove_file(char *);
extern "C" bool create_dir(char *);
extern "C" bool create_file(char *);
extern "C" bool rename_(char *, char *);
extern "C" char *append(char *, char *);
extern "C" bool copy(char *, char *);
extern "C" char *get_absolute(char *);
extern "C" char *parent(char *);
extern "C" char *file_name(char *);
extern "C" char *get_extension(char *);
extern "C" char *get_user_home();
extern "C" char *get_data_folder();
extern "C" char *get_config_folder();
extern "C" char *get_working_dir();
extern "C" char *project_dir(char *, char *, char *);
extern "C" bool *set_working_dir(char *);
extern "C" uint64_t get_file_size(char *);
extern "C" void drop_cstring(char *);
} // namespace rfm_

namespace FileManager {
std::vector<uint8_t> read_to_vec(const std::string &path);

void write_to_file(const std::string &path,
                   const std::vector<uint8_t> &contents);

void append_to_file(const std::string &path,
                    const std::vector<uint8_t> &contents);

void append_to_file(const std::string &path, uint8_t *contents, size_t size);

void append_to_file(const std::string &path, Serialized contents);

struct Path {
private:
  static std::string get_string(char *rs_cstring) {
    std::string result(rs_cstring);
    rfm_::drop_cstring(rs_cstring);

    return result;
  }

public:
  std::string path;

  explicit Path(std::string path) : path(std::move(path)) {}

  Path operator/(std::string const &other) const {
    std::string new_path(
        get_string(rfm_::append((char *)path.c_str(), (char *)other.c_str())));
    return Path{new_path};
  }

  Path operator/(Path const &other) const {
    std::string new_path(get_string(
        rfm_::append((char *)path.c_str(), (char *)other.path.c_str())));
    return Path{new_path};
  }

  Path operator+(std::string const &other) const {
    std::string new_path(
        get_string(rfm_::append((char *)path.c_str(), (char *)other.c_str())));
    return Path{new_path};
  }

  Path operator+(Path const &other) const {
    std::string new_path(get_string(
        rfm_::append((char *)path.c_str(), (char *)other.path.c_str())));
    return Path{new_path};
  }

  Path &operator/=(std::string const &other) {
    path =
        get_string(rfm_::append((char *)path.c_str(), (char *)other.c_str()));
    return *this;
  }

  Path &operator/=(Path const &other) {
    path = get_string(
        rfm_::append((char *)path.c_str(), (char *)other.path.c_str()));
    return *this;
  }

  Path &operator+=(std::string const &other) {
    path =
        get_string(rfm_::append((char *)path.c_str(), (char *)other.c_str()));
    return *this;
  }

  Path &operator+=(Path const &other) {
    path = get_string(
        rfm_::append((char *)path.c_str(), (char *)other.path.c_str()));
    return *this;
  }

  Path &operator=(std::string const &other) {
    path = other;
    return *this;
  }

  bool operator==(Path const &other) const { return path == other.path; }

  bool operator!=(Path const &other) const { return path != other.path; }

  static Path get_project_dir(std::string const &project_name,
                              std::string const &author_name,
                              std::string const &version) {
    std::string path(get_string(rfm_::project_dir((char *)project_name.c_str(),
                                                  (char *)author_name.c_str(),
                                                  (char *)version.c_str())));
    return Path{path};
  }

  static Path get_working_dir() {
    return Path{get_string(rfm_::get_working_dir())};
  }

  static Path get_user_home() {
    return Path{get_string(rfm_::get_user_home())};
  }

  static Path get_data_folder() {
    return Path{get_string(rfm_::get_data_folder())};
  }

  static Path get_config_folder() {
    return Path{get_string(rfm_::get_config_folder())};
  }

  [[nodiscard]] Path get_absolute() const {
    return Path{get_string(rfm_::get_absolute((char *)path.c_str()))};
  }

  [[nodiscard]] Path get_parent() const {
    return Path{get_string(rfm_::parent((char *)path.c_str()))};
  }

  [[nodiscard]] std::string get_file_name() const {
    return {get_string(rfm_::file_name((char *)path.c_str()))};
  }

  [[nodiscard]] std::string get_extension() const {
    return {get_string(rfm_::get_extension((char *)path.c_str()))};
  }

  [[nodiscard]] bool exists() const {
    return rfm_::exists((char *)path.c_str());
  }

  [[nodiscard]] bool is_dir() const {
    return rfm_::is_dir((char *)path.c_str());
  }

  [[nodiscard]] bool is_file() const {
    return rfm_::is_file((char *)path.c_str());
  }

  [[nodiscard]] bool is_symlink() const {
    return rfm_::is_symlink((char *)path.c_str());
  }

  [[nodiscard]] bool is_absolute() const {
    return rfm_::is_absolute((char *)path.c_str());
  }

  [[nodiscard]] bool is_relative() const {
    return rfm_::is_relative((char *)path.c_str());
  }

  static bool remove_dir(Path const &path) {
    return rfm_::remove_dir((char *)path.path.c_str());
  }

  static bool remove_file(Path const &path) {
    return rfm_::remove_file((char *)path.path.c_str());
  }

  static std::optional<Path> create_dir(std::string const &path) {
    if (rfm_::create_dir((char *)path.c_str())) {
      return std::make_optional<Path>(path);
    } else {
      return std::nullopt;
    }
  }

  static std::optional<Path> create_file(std::string const &path) {
    if (rfm_::create_file((char *)path.c_str())) {
      return std::make_optional<Path>(path);
    } else {
      return std::nullopt;
    }
  }

  [[nodiscard]] bool remove() const {
    if (is_dir()) {
      return rfm_::remove_dir((char *)path.c_str());
    } else if (is_file()) {
      return rfm_::remove_file((char *)path.c_str());
    } else {
      return false;
    }
  }

  [[nodiscard]] bool create_as_dir() const {
    return rfm_::create_dir((char *)path.c_str());
  }

  [[nodiscard]] bool create_as_file() const {
    return rfm_::create_file((char *)path.c_str());
  }

  bool rename_file(Path const &new_path) {
    auto result =
        rfm_::rename_((char *)path.c_str(), (char *)new_path.path.c_str());
    if (result) {
      path = new_path.path;
    }
    return result;
  }

  bool rename_dir(Path const &new_path) {
    auto result =
        rfm_::rename_((char *)path.c_str(), (char *)new_path.path.c_str());
    if (result) {
      path = new_path.path;
    }
    return result;
  }

  [[nodiscard]] std::optional<Path> copy_file(Path const &new_path) const {
    auto result =
        rfm_::copy((char *)path.c_str(), (char *)new_path.path.c_str());
    if (result) {
      return std::make_optional<Path>(new_path);
    } else {
      return std::nullopt;
    }
  }

  static bool set_working_dir(Path const &path) {
    return rfm_::set_working_dir((char *)path.path.c_str());
  }

  [[nodiscard]] bool set_as_working_dir() const {
    return rfm_::set_working_dir((char *)path.c_str());
  }

  [[nodiscard]] uint64_t get_file_size() const {
    return rfm_::get_file_size((char *)path.c_str());
  }
};

template <typename T>
cpp::result<std::vector<Path>, std::string> list_dir(T path) {
  namespace fs = std::filesystem;

  std::vector<Path> result;

  Path p = Path(path);

  if (!p.is_dir()) {
    return cpp::fail(fmt::format("Path {} is not a directory", p.path));
  }

  for (const auto &entry : fs::directory_iterator(p.path)) {
    result.push_back(Path(entry.path().string()));
  }

  return result;
}

} // namespace FileManager

#endif // FM_HPP
