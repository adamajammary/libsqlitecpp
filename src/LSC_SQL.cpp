#include "LSC_SQL.h"

int LSC_SQL::Execute(const std::string& query)
{
	int  result;
	auto connection = LSC_GetDBConnection();

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_exec(connection, query.c_str(), nullptr, nullptr, nullptr);

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if (result != SQLITE_OK)
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return result;
}

int LSC_SQL::Execute(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return result;
}

int LSC_SQL::Finalize(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_finalize(statement);

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if (result != SQLITE_OK)
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return result;
}

sqlite3_stmt* LSC_SQL::GetPreparedStatement(const std::string& query)
{
	sqlite3_stmt* statement  = nullptr;
	auto          connection = LSC_GetDBConnection();

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		auto result = sqlite3_prepare(connection, query.c_str(), -1, &statement, nullptr);

		#if defined _DEBUG
		if (result != SQLITE_OK)
			printf("%d: %s\n", result, sqlite3_errstr(result));
		#endif

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	return statement;
}

int LSC_SQL::GetResultInt(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if (result == SQLITE_ROW)
			return sqlite3_column_int(statement, 0);

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return 0;
}

LSC_TableRow LSC_SQL::getRow(sqlite3_stmt* statement)
{
	LSC_TableRow row;

	for (int i = 0; i < sqlite3_column_count(statement); i++)
	{
		auto name  = sqlite3_column_name(statement, i);
		auto value = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));

		if (name)
			row[name] = (value ? value : "");
	}

	return row;
}

LSC_TableRow LSC_SQL::GetRow(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if (result == SQLITE_ROW)
			return LSC_SQL::getRow(statement);

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return {};
}

bool LSC_SQL::GetRowExists(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if (result == SQLITE_ROW)
			return (sqlite3_column_int(statement, 0) == 1);

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return false;
}

LSC_TableRows LSC_SQL::GetRows(sqlite3_stmt* statement)
{
	int           result;
	LSC_TableRows rows;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if (result == SQLITE_ROW) {
			rows.push_back(LSC_SQL::getRow(statement));
			i = 0;
			continue;
		}

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return rows;
}

std::string LSC_SQL::GetValue(sqlite3_stmt* statement)
{
	int result;

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		result = sqlite3_step(statement);
	
		if (result == SQLITE_ROW) {
			auto column = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
			return (column ? std::string(column) : "");
		}

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	#if defined _DEBUG
	if ((result != SQLITE_DONE) && (result != SQLITE_OK))
		printf("%d: %s\n", result, sqlite3_errstr(result));
	#endif

	return "";
}

// https://cplusplus.com/reference/regex/ECMAScript/

bool LSC_SQL::IsValid(const std::string& value)
{
	return std::regex_match(value, std::regex("^[A-Za-z]+\\w*$"));
}
