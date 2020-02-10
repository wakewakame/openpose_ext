#include <Utils/Database.h>

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
namespace SQLite
{
	/// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
	void assertion_failed(const char* apFile, const long apLine, const char* apFunc, const char* apExpr, const char* apMsg)
	{
		// Print a message to the standard error output stream, and abort the program.
		std::cerr << apFile << ":" << apLine << ":" << " error: assertion failed (" << apExpr << ") in " << apFunc << "() with message \"" << apMsg << "\"\n";
		std::abort();
	}
}
#endif

#ifdef _WIN32
#include <Windows.h>
template<UINT SrcCodePage, UINT DstCodePage>
std::string convertStringCodePage(const std::string& src)
{
	int lenghtUnicode = MultiByteToWideChar(SrcCodePage, 0, src.c_str(), (int)src.size(), NULL, 0);
	if (lenghtUnicode == 0) throw(std::out_of_range("ERROR_NO_UNICODE_TRANSLATION"));
	std::wstring bufUnicode(lenghtUnicode, L'\0');
	int lenghtUnicodeActual = MultiByteToWideChar(SrcCodePage, 0, src.c_str(), (int)src.size(), &bufUnicode[0], (int)bufUnicode.size());
	assert(lenghtUnicode == lenghtUnicodeActual);
	int lengthDst = WideCharToMultiByte(DstCodePage, 0, &bufUnicode[0], (int)bufUnicode.size(), NULL, 0, NULL, NULL);
	if (lengthDst == 0) throw(std::out_of_range("ERROR_NO_UNICODE_TRANSLATION"));
	std::string dst(lengthDst, '\0');
	int lengthDstActual = WideCharToMultiByte(DstCodePage, 0, &bufUnicode[0], (int)bufUnicode.size(), &dst[0], (int)dst.size(), NULL, NULL);
	assert(lengthDst == lengthDstActual);
	return dst;
}
std::string toUTF8(const std::string& src) { return convertStringCodePage<CP_THREAD_ACP, CP_UTF8>(src); }
std::string fromUTF8(const std::string& src) { return convertStringCodePage<CP_UTF8, CP_THREAD_ACP>(src); }
#else
std::string toUTF8(const std::string& src) { return src; }
std::string fromUTF8(const std::string& src) { return src; }
#endif

Database::Database() {}

Database::~Database()
{
	upTransaction->commit();
}

int Database::create(const std::string& path, const int aFlags)
{
	try
	{
		// sqlファイルの作成
		database = std::make_shared<SQLite::Database>(toUTF8(path), aFlags);

		// トランザクションの開始
		upTransaction = std::make_unique<SQLite::Transaction>(*database);
	}
	catch (const std::exception & e)
	{
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}

	return 0;
}

int Database::commit()
{
	try
	{
		upTransaction->commit();
		upTransaction = std::make_unique<SQLite::Transaction>(*database);
	}
	catch (const std::exception & e)
	{
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}

	return 0;
}

int Database::createTableIfNoExist(const std::string& tableName, const std::string& rowTitles)
{
	try
	{
		if (!isDataExist("sqlite_master", "type", "name", "table", tableName))
		{
			database->exec(u8"CREATE TABLE " + tableName + " (" + rowTitles + u8")");
		}
	}
	catch (const std::exception & e)
	{
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}

	return 0;
}

int Database::createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle, bool isUnique)
{
	try
	{
		std::string indexName = "idx_" + rowTitle + u8"_on_" + tableName;
		if (!isDataExist("sqlite_master", "type", "name", "index", indexName))
		{
			database->exec(u8"CREATE" + std::string(isUnique ? u8" UNIQUE" : u8"") + " INDEX " + indexName + u8" ON " + tableName + u8"(" + rowTitle + u8")");
		}
	}
	catch (const std::exception & e)
	{
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}

	return 0;
}

int Database::createIndexIfNoExist(const std::string& tableName, const std::string& rowTitle1, const std::string& rowTitle2, bool isUnique)
{
	try
	{
		std::string indexName = "idx_" + rowTitle1 + u8"_and_" + rowTitle2 + u8"_on_" + tableName;
		if (!isDataExist("sqlite_master", "type", "name", "index", indexName))
		{
			database->exec(u8"CREATE" + std::string(isUnique ? u8" UNIQUE" : u8"") + " INDEX " + indexName + u8" ON " + tableName + u8"(" + rowTitle1 + u8", " + rowTitle2 + u8")");
		}
	}
	catch (const std::exception & e)
	{
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}

	return 0;
}

bool Database::isDataExist(const  std::string& tableName, const  std::string& rowTitle, long long number)
{
	SQLite::Statement timestampQuery(*database, u8"SELECT count(*) FROM " + tableName + " WHERE " + rowTitle + "=?");
	timestampQuery.bind(1, (long long)number);
	(void)timestampQuery.executeStep();
	return (1 == timestampQuery.getColumn(0).getInt());
}

bool Database::isDataExist(const  std::string& tableName, const  std::string& rowTitle1, std::string rowTitle2, long long number1, long long number2)
{
	SQLite::Statement timestampQuery(*database, u8"SELECT count(*) FROM " + tableName + " WHERE " + rowTitle1 + "=? AND " + rowTitle2 + "=?");
	timestampQuery.bind(1, (long long)number1);
	timestampQuery.bind(2, (long long)number2);
	(void)timestampQuery.executeStep();
	return (1 == timestampQuery.getColumn(0).getInt());
}

bool Database::isDataExist(const  std::string& tableName, const  std::string& rowTitle, const  std::string& text)
{
	SQLite::Statement timestampQuery(*database, u8"SELECT count(*) FROM " + tableName + " WHERE " + rowTitle + "=?");
	timestampQuery.bind(1, text);
	(void)timestampQuery.executeStep();
	return (1 == timestampQuery.getColumn(0).getInt());
}

bool Database::isDataExist(const  std::string& tableName, const  std::string& rowTitle1, const  std::string& rowTitle2, const  std::string& text1, const  std::string& text2)
{
	SQLite::Statement timestampQuery(*database, u8"SELECT count(*) FROM " + tableName + " WHERE " + rowTitle1 + "=? AND " + rowTitle2 + "=?");
	timestampQuery.bind(1, text1);
	timestampQuery.bind(2, text2);
	(void)timestampQuery.executeStep();
	return (1 == timestampQuery.getColumn(0).getInt());
}