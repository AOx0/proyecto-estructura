#include <gtest/gtest.h>

#include "../src/lib/fm.hpp"

TEST(Fm, Absolutize) {
// Expect two strings not to be equal.
std::string path ("../.");
std::string absolute (FileManager::Path::get_absolute(const_cast<char *>(path.c_str())));
// Expect equality.
EXPECT_EQ("/Users/alejandro/CLionProjects/proyecto-estructura", absolute);
}

TEST(Fm, Exists) {
// Expect two strings not to be equal.
  std::string path ("../src");
  bool exists (FileManager::Path::exists(const_cast<char *>(path.c_str())));
// Expect equality.
  EXPECT_EQ(true, exists);
}

TEST(Fm, UserDir) {
// Expect two strings not to be equal.
  std::string home (FileManager::Path::get_user_home());
// Expect equality.
  EXPECT_EQ("/Users/alejandro", home);
}

TEST(Fm, GetSetCurrent) {
// Expect two strings not to be equal.
  std::string current (FileManager::Path::get_working_dir());
  std::string new_current (FileManager::Path::get_parent(const_cast<char *>(current.c_str())));

  std::string parent (FileManager::Path::get_parent(const_cast<char *>(current.c_str())));
// Expect equality.
  EXPECT_EQ("/Users/alejandro/CLionProjects/proyecto-estructura", parent);
  EXPECT_EQ("/Users/alejandro/CLionProjects/proyecto-estructura", new_current);
}