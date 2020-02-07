#pragma once

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

std::shared_ptr<SQLite::Database> createDatabase(const std::string& path, const int aFlags);
using Database = std::shared_ptr<SQLite::Database>;