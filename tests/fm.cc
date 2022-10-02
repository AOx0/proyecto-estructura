#include <gtest/gtest.h>

#include "../src/lib/fm.hpp"

using namespace FileManager::Path;

/* Tested locally
TEST(Fm, Absolutize) {
// Expect two strings not to be equal.
std::string path ("../.");
std::string absolute (get_absolute(const_cast<char *>(path.c_str())));
// Expect equality.
EXPECT_EQ("/Users/alejandro/CLionProjects/proyecto-estructura", absolute);
}
*/

TEST(Fm, Exists) {
// Expect two strings not to be equal.
  std::string path ("../src");
  bool file_exists (exists(const_cast<char *>(path.c_str())));
// Expect equality.
  EXPECT_EQ(true, file_exists);
}

/* Tested locally
TEST(Fm, UserDir) {
// Expect two strings not to be equal.
  std::string home (FileManager::Path::get_user_home());
// Expect equality.
  EXPECT_EQ("/Users/alejandro", home);
}
 */

TEST(Fm, GetSetCurrent) {
// Expect two strings not to be equal.
  std::string current (get_working_dir());
  std::string current_parent (get_parent(const_cast<char *>(current.c_str())));
  bool success (set_working_dir(const_cast<char *>(current_parent.c_str())));

  if (success) {
    std::string new_current (get_working_dir());
    EXPECT_EQ(current_parent, new_current);
  } else {
    EXPECT_EQ(true, success);
  }
}