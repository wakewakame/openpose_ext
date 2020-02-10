#pragma once

#include <iostream>
#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

class Database
{
private:
	std::unique_ptr<SQLite::Transaction> upTransaction;

	void bind(SQLite::Statement&, size_t);
	template<typename Head, typename... Body>
	void bind(SQLite::Statement& query, size_t index, Head head, Body... body)
	{
		query.bind(index, head);
		bind(query, index + 1, body...);
	}

	template<typename... Body> void bind(SQLite::Statement& query, size_t index, uint64_t head, Body... body) { bind(query, index, (long long)head, body...); }
	template<typename... Body> void bind(SQLite::Statement& query, size_t index, float head, Body... body) { bind(query, index, (double)head, body...); }

public:
	Database();

	virtual ~Database();

	std::shared_ptr<SQLite::Database> database;

	int create(const std::string& path, const int aFlags);

	int commit();

	int createTableIfNoExist(const std::string& tableName, const std::string& rowTitles);

	int createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle, bool isUnique);

	int createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle1, const std::string& rowTitle2, bool isUnique);

	int deleteTableIfExist(const std::string& tableName);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle, long long number);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle1, std::string rowTitle2, long long number1, long long number2);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle, const  std::string& text);

	bool isDataExist(const  std::string& tableName, const  std::string& rowTitle1, const  std::string& rowTitle2, const  std::string& text1, const  std::string& text2);
	
	template<typename... Body>
	int bindAll(SQLite::Statement& query, Body... body)
	{
		try
		{
			query.reset();
			bind(query, 1, body...);
		}
		catch (const std::exception & e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}
	template<typename... Body>
	int bindAllAndExec(SQLite::Statement& query, Body... body)
	{
		try
		{
			if (bindAll(query, body...)) return 1;
			(void)query.exec();
		}
		catch (const std::exception & e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}
};