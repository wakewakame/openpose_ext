#include "Utils/Database.h"

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
	int lenghtUnicode = MultiByteToWideChar(SrcCodePage, 0, src.c_str(), src.size(), NULL, 0);
	if (lenghtUnicode == 0) throw(std::out_of_range("ERROR_NO_UNICODE_TRANSLATION"));
	std::wstring bufUnicode(lenghtUnicode, L'\0');
	int lenghtUnicodeActual = MultiByteToWideChar(SrcCodePage, 0, src.c_str(), src.size(), &bufUnicode[0], bufUnicode.size());
	assert(lenghtUnicode == lenghtUnicodeActual);
	int lengthDst = WideCharToMultiByte(DstCodePage, 0, &bufUnicode[0], bufUnicode.size(), NULL, 0, NULL, NULL);
	if (lengthDst == 0) throw(std::out_of_range("ERROR_NO_UNICODE_TRANSLATION"));
	std::string dst(lengthDst, L'\0');
	int lengthDstActual = WideCharToMultiByte(DstCodePage, 0, &bufUnicode[0], bufUnicode.size(), &dst[0], dst.size(), NULL, NULL);
	assert(lengthDst == lengthDstActual);
	return dst;
}
std::string toUTF8(const std::string& src) { return convertStringCodePage<CP_THREAD_ACP, CP_UTF8>(src); }
std::string fromUTF8(const std::string& src) { return convertStringCodePage<CP_UTF8, CP_THREAD_ACP>(src); }
#else
std::string toUTF8(const std::string& src) { return src; }
std::string fromUTF8(const std::string& src) { return src; }
#endif

std::shared_ptr<SQLite::Database> createDatabase(const std::string& path)
{
	return std::make_shared<SQLite::Database>(toUTF8(path), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
}