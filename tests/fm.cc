#include <gtest/gtest.h>

#include "../src/lib/fm.hpp"

TEST(Fm, Exists) {
  FileManager::Path path ("../src");
  bool file_exists (path.exists());
  EXPECT_EQ(true, file_exists);
}

TEST(Fm, GetSetCurrent) {
  FileManager::Path current (FileManager::Path::get_working_dir());
  FileManager::Path current_parent (current.get_parent());
  bool success (current_parent.set_as_working_dir());

  if (success) {
    FileManager::Path new_current (FileManager::Path::get_working_dir());
    EXPECT_EQ(current_parent, new_current);
  } else {
    EXPECT_EQ(true, success);
  }
}