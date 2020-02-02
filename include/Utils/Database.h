#pragma once

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

std::shared_ptr<SQLite::Database> createDatabase(const std::string& path);
using Database = std::shared_ptr<SQLite::Database>;