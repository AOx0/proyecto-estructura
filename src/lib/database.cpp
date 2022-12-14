#include "database.hpp"

#include "fm.hpp"

bool DataBase::operator==(const DataBase &other) const {
  return nombre == other.nombre;
}

cpp::result<DataBase, std::string> DataBase::create(const std::string &name) {
  using P = FileManager::Path;

  DataBase d({});

  // We attempt to re-create the data folder. Just in case
  auto p(P::create_dir("data/"));

  if (p.has_value()) {
    *p = *p / name;
    if (p->exists() && p->is_dir()) {
      return cpp::fail("Database already exists");
    } else {
      auto result = p->create_as_dir();
      if (!result)
        return cpp::fail(fmt::format("Failed to create directory {}", p->path));
      return d;
    }
  } else {
    return cpp::fail("Failed getting path for the database");
  }

  return cpp::fail("Something went bad while creating database");
}

cpp::result<void, std::string> DataBase::delete_table_dir(std::string database,
                                                          std::string table) {
  auto db_path = FileManager::Path("data") / database / table;
  if (db_path.exists() && db_path.is_dir()) {
    if (!db_path.remove()) {
      return cpp::fail(fmt::format("Failed to remove folder {}", db_path.path));
    }
  }

  return {};
}