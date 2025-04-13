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
		sqlite3_bind_text(statement, position, column.value.c_str(), -1, nullptr);
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

    std::string selectColumns = "";

    for (size_t i = 0; i < query.selectColumns.size(); i++)
    {
        if (!LSC_SQL::IsValid(query.selectColumns[i]))
            throw std::runtime_error(TextFormat("Invalid SELECT column name '%s'.", query.selectColumns[i].c_str()));

        if (!query.search.empty() && (query.selectColumns[i] == "id"))
            selectColumns.append("rowid");
        else
            selectColumns.append(query.selectColumns[i]);

        if (i < (query.selectColumns.size() - 1))
            selectColumns.append(", ");
    }

	bool hasWhere       = !query.whereCondition.columns.empty();
	auto whereCondition = LSC_SQL::GetWhereCondition(query.whereCondition);

	std::string whereClause = "";

	if (!query.search.empty() && hasWhere)
		whereClause = TextFormat(" WHERE %s_fts MATCH ? AND %s", query.table.c_str(), whereCondition.c_str());
    else if (!query.search.empty())
		whereClause = TextFormat(" WHERE %s_fts MATCH ?", query.table.c_str());
    else if (hasWhere)
		whereClause = TextFormat(" WHERE %s", whereCondition.c_str());

	bool hasOrderBy = !query.orderByColumns.empty();

	std::string orderBy = "";

	if (hasOrderBy)
	{
		orderBy = " ORDER BY ";

		for (size_t i = 0; i < query.orderByColumns.size(); i++)
		{
			if (!LSC_SQL::IsValid(query.orderByColumns[i].name))
				throw std::runtime_error(TextFormat("Invalid ORDER BY column name '%s'.", query.orderByColumns[i].name.c_str()));

			orderBy.append(TextFormat("%s %s", query.orderByColumns[i].name.c_str(), (query.orderByColumns[i].isDescending ? "DESC" : "ASC")));

			if (i < (query.orderByColumns.size() - 1))
				orderBy.append(", ");
		}
	}

	auto distinct = (query.isDistinct ? "DISTINCT " : "");
    auto columns  = (!query.selectColumns.empty() ? selectColumns.c_str() : "*");
    auto table    = (!query.search.empty() ? TextFormat("%s_fts", query.table.c_str()) : query.table);

	std::string select;

	if (noLimit)
		select = TextFormat("SELECT %s%s FROM %s%s%s", distinct, columns, table.c_str(), whereClause.c_str(), orderBy.c_str());
	else
		select = TextFormat("SELECT %s%s FROM %s%s%s LIMIT %d OFFSET %d;", distinct, columns, table.c_str(), whereClause.c_str(), orderBy.c_str(), query.limit, query.offset);

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

std::string LSC_SQL::GetWhereCondition(const LSC_WhereCondition& whereCondition)
{
	if (whereCondition.columns.empty())
		return "";

	std::string result = "";

	for (size_t i = 0; i < whereCondition.columns.size(); i++)
	{
		if (!LSC_SQL::IsValid(whereCondition.columns[i].name))
			throw std::runtime_error(TextFormat("Invalid WHERE column name '%s'.", whereCondition.columns[i].name.c_str()));

		auto comparison = LSC_SQL::getComparison(whereCondition.columns[i].comparison);

		result.append(TextFormat("%s%s?", whereCondition.columns[i].name.c_str(), comparison.c_str()));

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
