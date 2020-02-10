#pragma once

#include <iostream>
#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

class Database
{
private:
	std::unique_ptr<SQLite::Transaction> upTransaction;

public:
	Database();

	virtual ~Database();

	std::shared_ptr<SQLite::Database> database;

	int create(const std::string& path, const int aFlags);

	int commit();

	int createTableIfNoExist(const std::string& tableName, const std::string& rowTitles);

	int createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle, bool isUnique);

	int createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle1, const std::string& rowTitle2, bool isUnique);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle, long long number);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle1, std::string rowTitle2, long long number1, long long number2);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle, const  std::string& text);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle1, const  std::string& rowTitle2, const  std::string& text1, const  std::string& text2);
};