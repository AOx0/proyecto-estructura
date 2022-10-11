#include "fm.hpp"

std::vector<uint8_t> FileManager::read_to_vec(const std::string &path) {
  std::fstream file;
  file.open(path, std::ios::in | std::ios::binary);
  file.unsetf(std::ios::skipws);

  std::streampos fileSize;

  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> vec;
  vec.reserve(fileSize);

  vec.insert(vec.begin(), std::istream_iterator<uint8_t>(file),
             std::istream_iterator<uint8_t>());

  file.close();
  return vec;
}

void FileManager::write_to_file(const std::string &path,
                                const std::vector<uint8_t> &contents) {
  std::ofstream file(path, std::ios::out | std::ios::binary);

  file.write((const char *) &contents[0], (std::streamsize) contents.size());
  file.close();
}

void FileManager::append_to_file(const std::string &path,
                                 const std::vector<unsigned char> &contents) {
  std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::app);

  file.write((const char *) &contents[0], (std::streamsize) contents.size());
  file.close();
}
