#include "main.h"

#ifndef LSC_SQL_H
#define LSC_SQL_H

class LSC_SQL
{
private:
	LSC_SQL()  {}
	~LSC_SQL() {}

private:
	static const int maxRetries = 10;

public:
	static int           Execute(const std::string& query);
	static int           Execute(sqlite3_stmt* statement);
	static int           Finalize(sqlite3_stmt* statement);
	static sqlite3_stmt* GetPreparedStatement(const std::string& query);
	static std::string   GetQueryFTS(const std::string& table, const std::vector<std::string>& columns);
	static std::string   GetQueryTriggerDelete(const std::string& table, const std::vector<std::string>& columns);
	static std::string   GetQueryTriggerInsert(const std::string& table, const std::vector<std::string>& columns);
	static std::string   GetQueryTriggerUpdate(const std::string& table, const std::vector<std::string>& columns);
	static int           GetResultInt(sqlite3_stmt* statement);
	static LSC_TableRow  GetRow(sqlite3_stmt* statement);
	static bool          GetRowExists(sqlite3_stmt* statement);
	static LSC_TableRows GetRows(sqlite3_stmt* statement);
	static std::string   GetValue(sqlite3_stmt* statement);
	static bool          IsValid(const std::string& value);

private:
	static LSC_TableRow getRow(sqlite3_stmt* statement);
};

#endif
