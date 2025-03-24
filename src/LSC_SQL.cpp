#include "LSC_SQL.h"

void LSC_SQL::Bind(const LSC_Query& query, sqlite3_stmt* statement)
{
	auto searchValue = (!query.search.empty() ? std::regex_replace(query.search, std::regex("(\\S+)"), "$&*") : "");

	if (!searchValue.empty())
		sqlite3_bind_text(statement, 1, searchValue.c_str(), -1, SQLITE_TRANSIENT);

	if (!query.whereColumn.name.empty())
		sqlite3_bind_text(statement, (!query.search.empty() ? 2 : 1), query.whereColumn.value.c_str(), -1, nullptr);
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
	auto query = TextFormat(
		"CREATE VIRTUAL TABLE IF NOT EXISTS %s_fts USING FTS5 (content=%s, content_rowid=id",
		table.c_str(), table.c_str()
	);

	for (const auto& column : columns)
		query.append(TextFormat(", %s", column.name.c_str()));

	query.append(");");

	return query;
}

std::string LSC_SQL::GetQueryTriggerDelete(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = TextFormat(
		"CREATE TRIGGER IF NOT EXISTS %s_fts_delete AFTER DELETE ON %s BEGIN INSERT INTO %s_fts (%s_fts, rowid",
		table.c_str(), table.c_str(), table.c_str(), table.c_str()
	);

	for (const auto& column : columns)
		query.append(TextFormat(", %s", column.name.c_str()));

	query.append(") VALUES ('delete', OLD.id");

	for (const auto& column : columns)
		query.append(TextFormat(", OLD.%s", column.name.c_str()));

	query.append("); END;");

	return query;
}

std::string LSC_SQL::GetQueryTriggerInsert(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = TextFormat(
		"CREATE TRIGGER IF NOT EXISTS %s_fts_insert AFTER INSERT ON %s BEGIN INSERT INTO %s_fts (rowid",
		table.c_str(), table.c_str(), table.c_str()
	);

	for (const auto& column : columns)
		query.append(TextFormat(", %s", column.name.c_str()));

	query.append(") VALUES (NEW.id");

	for (const auto& column : columns)
		query.append(TextFormat(", NEW.%s", column.name.c_str()));

	query.append("); END;");

	return query;
}

std::string LSC_SQL::GetQueryTriggerUpdate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
	auto query = TextFormat(
		"CREATE TRIGGER IF NOT EXISTS %s_fts_update AFTER UPDATE ON %s BEGIN INSERT INTO %s_fts (%s_fts, rowid",
		table.c_str(), table.c_str(), table.c_str(), table.c_str()
	);

	for (const auto& column : columns)
		query.append(TextFormat(", %s", column.name.c_str()));

	query.append(") VALUES ('delete', OLD.id");

	for (const auto& column : columns)
		query.append(TextFormat(", OLD.%s", column.name.c_str()));

	query.append(TextFormat("); INSERT INTO %s_fts (rowid", table.c_str()));

	for (const auto& column : columns)
		query.append(TextFormat(", %s", column.name.c_str()));

	query.append(") VALUES (NEW.id");

	for (const auto& column : columns)
		query.append(TextFormat(", NEW.%s", column.name.c_str()));

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
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", query.table.c_str()));

    bool hasOrderBy = !query.orderByColumn.name.empty();

    if (hasOrderBy && !LSC_SQL::IsValid(query.orderByColumn.name))
        throw std::runtime_error(TextFormat("Invalid orderBy column name '%s'.", query.orderByColumn.name.c_str()));

    bool hasWhere = !query.whereColumn.name.empty();

    if (hasWhere && !LSC_SQL::IsValid(query.whereColumn.name))
        throw std::runtime_error(TextFormat("Invalid where column name '%s'.", query.whereColumn.name.c_str()));

    bool        hasSelectColumns = !query.selectColumns.empty();
    std::string selectColumns    = "";

    for (size_t i = 0; i < query.selectColumns.size(); i++)
    {
        if (!LSC_SQL::IsValid(query.selectColumns[i]))
            throw std::runtime_error(TextFormat("Invalid column name '%s'.", query.selectColumns[i].c_str()));

        if (!query.search.empty() && (query.selectColumns[i] == "id"))
            selectColumns.append("rowid");
        else
            selectColumns.append(query.selectColumns[i]);

        if (i < (query.selectColumns.size() - 1))
            selectColumns.append(", ");
    }

    std::string filter = "";

    if (!query.search.empty() && hasWhere)
        filter = TextFormat(" WHERE %s_fts MATCH ? AND %s=?", query.table.c_str(), query.whereColumn.name.c_str());
    else if (!query.search.empty())
        filter = TextFormat(" WHERE %s_fts MATCH ?", query.table.c_str());
    else if (hasWhere)
        filter = TextFormat(" WHERE %s=?", query.whereColumn.name.c_str());

    auto distinct = (query.isDistinct ? "DISTINCT " : "");
    auto columns  = (hasSelectColumns ? selectColumns.c_str() : "*");
    auto orderBy  = (hasOrderBy ? TextFormat(" ORDER BY %s %s", query.orderByColumn.name.c_str(), (query.orderByColumn.isDescending ? "DESC" : "ASC")) : "");
    auto table    = (!query.search.empty() ? TextFormat("%s_fts", query.table.c_str()) : query.table);

	std::string select;

	if (noLimit)
		select = TextFormat("SELECT %s%s FROM %s%s%s", distinct, columns, table.c_str(), filter.c_str(), orderBy.c_str());
	else
		select = TextFormat("SELECT %s%s FROM %s%s%s LIMIT %d OFFSET %d;", distinct, columns, table.c_str(), filter.c_str(), orderBy.c_str(), query.limit, query.offset);

	return select;
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
