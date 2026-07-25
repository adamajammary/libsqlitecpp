#include "LSC_SQL.h"

void LSC_SQL::Bind(const LSC_ColumnValue& column, int position, sqlite3_stmt* statement)
{
	switch (column.type) {
	case LSC_DATA_TYPE_FLOAT:
		sqlite3_bind_double(statement, position, std::atof(column.value.c_str()));
		break;
	case LSC_DATA_TYPE_INTEGER:
		sqlite3_bind_int64(statement, position, std::atoll(column.value.c_str()));
		break;
	default:
		sqlite3_bind_text(statement, position, LSC_SQL::getTextValue(column).c_str(), -1, SQLITE_TRANSIENT);
		break;
	}
}

void LSC_SQL::Bind(const LSC_Query& query, sqlite3_stmt* statement)
{
	auto searchValue = (!query.search.empty() ? std::regex_replace(query.search, std::regex("(\\S+)"), "$&*") : "");

	if (!searchValue.empty())
		sqlite3_bind_text(statement, 1, searchValue.c_str(), -1, SQLITE_TRANSIENT);

	int position = (!query.search.empty() ? 2 : 1);

	for (int i = 0; i < (int)query.whereCondition.columns.size(); i++)
		LSC_SQL::Bind(query.whereCondition.columns[i], (position + i), statement);
}

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
		printf("%d: %s | %s\n", result, sqlite3_errstr(result), query.c_str());
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

std::string LSC_SQL::getComparison(LSC_Comparison comparison)
{
	switch (comparison) {
		case LSC_COMPARISON_CONTAINS: return " LIKE ";
		case LSC_COMPARISON_EQUALS: return "=";
		case LSC_COMPARISON_NOT_EQUALS: return "<>";
		case LSC_COMPARISON_IS_NULL: return " IS NULL ";
		case LSC_COMPARISON_IS_NOT_NULL: return " IS NOT NULL ";
		case LSC_COMPARISON_GREATER_THAN: return ">";
		case LSC_COMPARISON_GREATER_THAN_OR_EQUALS: return ">=";
		case LSC_COMPARISON_LESS_THAN: return "<";
		case LSC_COMPARISON_LESS_THAN_OR_EQUALS: return "<=";
		default: break;
	}

	return "";
}

std::string LSC_SQL::getOperation(LSC_Operation operation)
{
	switch (operation) {
		case LSC_OPERATION_AND: return " AND ";
		case LSC_OPERATION_OR:  return " OR ";
		default: break;
	}

	return "";
}

sqlite3_stmt* LSC_SQL::GetPreparedStatement(const std::string& query)
{
	sqlite3_stmt* statement  = nullptr;
	auto          connection = LSC_GetDBConnection();

	for (auto i = 0; i < LSC_SQL::maxRetries; i++)
	{
		auto result = sqlite3_prepare_v2(connection, query.c_str(), -1, &statement, nullptr);

		#if defined _DEBUG
		if (result != SQLITE_OK)
			printf("%d: %s | %s\n", result, sqlite3_errstr(result), query.c_str());
		#endif

		if ((result != SQLITE_BUSY) && (result != SQLITE_LOCKED))
			break;
	}

	return statement;
}

std::string LSC_SQL::GetQueryFTS(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = std::format(
		"CREATE VIRTUAL TABLE IF NOT EXISTS {}_fts USING FTS5 (content={}, content_rowid=id",
		table, table
	);

	for (const auto& column : columns)
		query.append(std::format(", {}", column.name));

	query.append(");");

	return query;
}

std::string LSC_SQL::GetQueryTriggerDelete(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = std::format(
		"CREATE TRIGGER IF NOT EXISTS {}_fts_delete AFTER DELETE ON {} BEGIN INSERT INTO {}_fts ({}_fts, rowid",
		table, table, table, table
	);

	for (const auto& column : columns)
		query.append(std::format(", {}", column.name));

	query.append(") VALUES ('delete', OLD.id");

	for (const auto& column : columns)
		query.append(std::format(", OLD.{}", column.name));

	query.append("); END;");

	return query;
}

std::string LSC_SQL::GetQueryTriggerInsert(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = std::format(
		"CREATE TRIGGER IF NOT EXISTS {}_fts_insert AFTER INSERT ON {} BEGIN INSERT INTO {}_fts (rowid",
		table, table, table
	);

	for (const auto& column : columns)
		query.append(std::format(", {}", column.name));

	query.append(") VALUES (NEW.id");

	for (const auto& column : columns)
		query.append(std::format(", NEW.{}", column.name));

	query.append("); END;");

	return query;
}

std::string LSC_SQL::GetQueryTriggerUpdate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = std::format(
		"CREATE TRIGGER IF NOT EXISTS {}_fts_update AFTER UPDATE ON {} BEGIN INSERT INTO {}_fts ({}_fts, rowid",
		table, table, table, table
	);

	for (const auto& column : columns)
		query.append(std::format(", {}", column.name));

	query.append(") VALUES ('delete', OLD.id");

	for (const auto& column : columns)
		query.append(std::format(", OLD.{}", column.name));

	query.append(std::format("); INSERT INTO {}_fts (rowid", table));

	for (const auto& column : columns)
		query.append(std::format(", {}", column.name));

	query.append(") VALUES (NEW.id");

	for (const auto& column : columns)
		query.append(std::format(", NEW.{}", column.name));

	query.append("); END;");

	return query;
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
			row[!std::strcmp(name, "rowid") ? "id" : name] = (value ? value : "");
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

std::string LSC_SQL::GetSelect(const LSC_Query& query, bool noLimit)
{
    if (!LSC_SQL::IsValid(query.table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", query.table));

    std::string columns = "";

    for (size_t i = 0; i < query.selectColumns.size(); i++)
    {
        if (!LSC_SQL::IsValid(query.selectColumns[i]))
            throw std::runtime_error(std::format("Invalid SELECT column name '{}'.", query.selectColumns[i]));

        if (!query.search.empty() && (query.selectColumns[i] == "id"))
			columns.append("rowid");
        else
			columns.append(query.selectColumns[i]);

        if (i < (query.selectColumns.size() - 1))
			columns.append(", ");
    }

	if (columns.empty())
		columns = (!query.search.empty() ? "rowid, *" : "*");

	bool hasWhere       = !query.whereCondition.columns.empty();
	auto whereCondition = LSC_SQL::GetWhereCondition(query.whereCondition);

	std::string whereClause = "";

	if (!query.search.empty() && hasWhere)
		whereClause = std::format(" WHERE {}_fts MATCH ? AND {}", query.table, whereCondition);
    else if (!query.search.empty())
		whereClause = std::format(" WHERE {}_fts MATCH ?", query.table);
    else if (hasWhere)
		whereClause = std::format(" WHERE {}", whereCondition);

	bool hasOrderBy = !query.orderByColumns.empty();

	std::string orderBy = "";

	if (hasOrderBy)
	{
		orderBy = " ORDER BY ";

		for (size_t i = 0; i < query.orderByColumns.size(); i++)
		{
			if (!LSC_SQL::IsValid(query.orderByColumns[i].name))
				throw std::runtime_error(std::format("Invalid ORDER BY column name '{}'.", query.orderByColumns[i].name));

			orderBy.append(std::format("{} {}", query.orderByColumns[i].name, (query.orderByColumns[i].isDescending ? "DESC" : "ASC")));

			if (i < (query.orderByColumns.size() - 1))
				orderBy.append(", ");
		}
	}

	auto distinct = (query.isDistinct ? "DISTINCT " : "");
    auto table    = (!query.search.empty() ? std::format("{}_fts", query.table) : query.table);

	std::string select;

	if (noLimit)
		select = std::format("SELECT {}{} FROM {}{}{}", distinct, columns, table, whereClause, orderBy);
	else
		select = std::format("SELECT {}{} FROM {}{}{} LIMIT {} OFFSET {};", distinct, columns, table, whereClause, orderBy, query.limit, query.offset);

	return select;
}

std::string LSC_SQL::getTextValue(const LSC_ColumnValue& column)
{
	if (column.comparison != LSC_COMPARISON_CONTAINS)
		return column.value;

	auto specialCharacters = std::regex("[%_]");

	if (std::regex_search(column.value, specialCharacters))
		return std::format("%{}%", std::regex_replace(column.value, specialCharacters, "\\$&"));

	return std::format("%{}%", column.value);
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

std::string LSC_SQL::GetWhereCondition(const LSC_WhereCondition& whereCondition)
{
	if (whereCondition.columns.empty())
		return "";

	std::string result = "";

	for (size_t i = 0; i < whereCondition.columns.size(); i++)
	{
		const auto& column = whereCondition.columns[i];

		if (!LSC_SQL::IsValid(column.name))
			throw std::runtime_error(std::format("Invalid WHERE column name '{}'.", column.name));

		auto comparison = LSC_SQL::getComparison(column.comparison);

		result.append(std::format("{}{}?", column.name, comparison));

		if ((column.comparison == LSC_COMPARISON_CONTAINS) && std::regex_search(column.value, std::regex("[%_]")))
			result.append(" ESCAPE '\\'");

		if (i < (whereCondition.columns.size() - 1))
			result.append(LSC_SQL::getOperation(whereCondition.operation));
	}

	return result;
}

// https://cplusplus.com/reference/regex/ECMAScript/

bool LSC_SQL::IsValid(const std::string& value)
{
	return std::regex_match(value, std::regex("^[A-Za-z]+\\w*$"));
}
