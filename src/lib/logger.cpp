#include "logger.hpp"

std::string Logger::show(LOG_TYPE_ type, const std::string &msg) {
  // match type
  std::string result;
  switch (type) {
  case INFO:
    result += fmt::format(/*fg(fmt::terminal_color::bright_white),*/ "INF :: ");
    break;
  case WARN:
    result += fmt::format(/*fg(fmt::terminal_color::yellow),*/ "WRN :: ");
    break;
  case ERROR:
    result += fmt::format(/*fg(fmt::terminal_color::red),*/ "ERR :: ");
    break;
  case NONE:
    break;
  }

  result += fmt::format(/*fg(fmt::terminal_color::bright_white),*/ "{}", msg);
  std::cout << result;
  return result;
}

std::string Logger::show_ln(LOG_TYPE_ type, const std::string &msg) {
  return show(type, msg + "\n");
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
    Path backup(path_.get_parent() + ("old_" + path_.get_file_name()));

    if (backup.exists()) {
      // remove and handle if error
      if (!backup.remove()) {
        show_ln(LOG_TYPE_::ERROR, "Error removing old backup file");
        exit(1);
      }
    }

    if (!rfm_::rename_((char *)path_.path.c_str(),
                       (char *)backup.path.c_str())) {
      show_ln(LOG_TYPE_::ERROR, "Error renaming log file");
      exit(1);
    }
  }

  // Check if path exists, create if not and set path_
  if (!path_.exists()) {
    if (!path_.create_as_file()) {
      show_ln(LOG_TYPE_::ERROR,
              fmt::format("Could not create log file at {}", path_.path));
      exit(1);
    }
  }
}

void Logger::log(const std::string &msg) {
  // Guard lock of mutex
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path,
                              to_vec(fmt::format("INF :: {}\n", msg)));
  show_ln(LOG_TYPE_::INFO, msg);
}

void Logger::warn(const std::string &msg) {
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path,
                              to_vec(fmt::format("WRN :: {}\n", msg)));
  show_ln(LOG_TYPE_::WARN, msg);
}

void Logger::error(const std::string &msg) {
  std::lock_guard<std::mutex> lock(mtx_);

  FileManager::append_to_file(path_.path,
                              to_vec(fmt::format("ERR :: {}\n", msg)));
  show_ln(LOG_TYPE_::ERROR, msg);
}
