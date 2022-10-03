#include "logger.hpp"

void Logger::show(LOG_TYPE_ type, const std::string &msg) {
  // match type
  switch (type) {
    case INFO:
      fmt::print(fg(fmt::terminal_color::bright_white), "INF :: ");
      break;
    case WARN:
      fmt::print(fg(fmt::terminal_color::yellow), "WRN :: ");
      break;
    case ERROR:
      fmt::print(fg(fmt::terminal_color::red), "ERR :: ");
      break;
  }

  fmt::print(fg(fmt::terminal_color::bright_white), "{}\n", msg);
}

std::vector<std::uint8_t> Logger::to_vec(const std::string &str) {
  std::vector<std::uint8_t> result;
  for (auto &c : str) {
    result.push_back(c);
  }
  return result;
}

Logger::Logger(Logger::Path path) : path_(std::move(path)) {
  if (path_.exists()) {
    Path backup (path_.get_parent() + ("old_" + path_.get_file_name()));

    std::cout << backup.path << std::endl;

    if (backup.exists()) {
      // remove and handle if error
      if (!backup.remove()) {
        show(LOG_TYPE_::ERROR, "Error removing old backup file" );
        exit(1);
      }
    }

    if (!rfm_::rename_file((char *) path_.path.c_str(), (char *) backup.path.c_str())) {
      show(LOG_TYPE_::ERROR, "Error renaming log file" );
      exit(1);
    }
  }

  // Check if path exists, create if not and set path_
  if (!path_.exists())  {
    if (!path_.create_as_file()){
      show(LOG_TYPE_::ERROR, fmt::format("Could not create log file at {}", path_.path) );
      exit(1);
    }
  }
}

void Logger::log(const std::string &msg) {
  // Guard lock of mutex
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path, to_vec(fmt::format("INF :: {}\n", msg)));
  show(LOG_TYPE_::INFO, msg);
}

void Logger::warn(const std::string &msg) {
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path, to_vec(fmt::format("WRN :: {}\n", msg)));
  show(LOG_TYPE_::WARN, msg);
}

void Logger::error(const std::string &msg) {
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path, to_vec(fmt::format("ERR :: {}\n", msg)));
  show(LOG_TYPE_::ERROR, msg);
}
