#include "main.h"

sqlite3* connection = nullptr;

sqlite3* LSC_GetDBConnection()
{
    return connection;
}

void LSC_CloseDatabase()
{
    if (connection)
        sqlite3_close(connection);
}

std::string LSC_Get(const std::string& key)
{
    return LSC_Settings::Get(key);
}

void LSC_OpenDatabase(const std::string& filePath)
{
    LSC_CloseDatabase();

    auto result = sqlite3_open(filePath.c_str(), &connection);

    if (result != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to open/write: %s", filePath.c_str()));

    LSC_Settings::Init();
}

void LSC_Set(const std::string& key, const std::string& value)
{
    LSC_Settings::Set(key, value);
}

void LSC_TableCreate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query = TextFormat("CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY", table.c_str());

    for (const auto& column : columns)
    {
        if (!LSC_SQL::IsValid(column.name))
            throw std::runtime_error(TextFormat("Invalid column name '%s'.", column.name.c_str()));

        auto notNull = (column.isNotNull    ? " NOT NULL"       : "");
        auto search  = (column.isSearchable ? " COLLATE NOCASE" : "");
        auto unique  = (column.isUnique     ? " UNIQUE"         : "");

        query.append(TextFormat(", %s TEXT%s%s%s", column.name.c_str(), unique, search, notNull));
    }
    
    query.append(");");

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create table '%s'.", table.c_str()));
}

void LSC_TableDelete(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query = TextFormat("DROP TABLE IF EXISTS %s;", table.c_str());

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop table '%s'.", table.c_str()));
}

void LSC_TableDeleteRow(const std::string& table, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query     = TextFormat("DELETE FROM %s WHERE id=?;", table.c_str());
    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    sqlite3_bind_int64(statement, 1, rowId);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);
}

int LSC_TableDeleteRow(const std::string& table, const LSC_ColumnValue& column)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    if (!LSC_SQL::IsValid(column.name))
        throw std::runtime_error(TextFormat("Invalid column name '%s'.", column.name.c_str()));

    auto query     = TextFormat("DELETE FROM %s WHERE %s=?;", table.c_str(), column.name.c_str());
    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    sqlite3_bind_text(statement, 1, column.value.c_str(), - 1, nullptr);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);

    return sqlite3_changes(connection);
}

void LSC_TableDeleteRows(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query = TextFormat("DELETE FROM %s;", table.c_str());

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to truncate table '%s'.", table.c_str()));
}

LSC_TableRow LSC_TableGetRow(const std::string& table, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query     = TextFormat("SELECT * FROM %s WHERE id=?;", table.c_str());
    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    sqlite3_bind_int64(statement, 1, rowId);

    auto row = LSC_SQL::GetRow(statement);

    LSC_SQL::Finalize(statement);

    return row;
}

size_t LSC_TableGetRowCount(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto select    = TextFormat("SELECT COUNT(*) FROM %s;", table.c_str());
	auto statement = LSC_SQL::GetPreparedStatement(select);

	if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

	auto count = (size_t)LSC_SQL::GetResultInt(statement);

	LSC_SQL::Finalize(statement);

	return count;
}

LSC_TableRows LSC_TableGetRows(const LSC_Query& query)
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

        selectColumns.append(query.selectColumns[i]).append(i < (query.selectColumns.size() - 1) ? ", " : "");
    }

    auto distinct  = (query.isDistinct ? "DISTINCT " : "");
    auto columns   = (hasSelectColumns ? selectColumns.c_str() : "*");
    auto filter    = (hasWhere   ? TextFormat(" WHERE %s=?", query.whereColumn.name.c_str()) : "");
    auto orderBy   = (hasOrderBy ? TextFormat(" ORDER BY %s %s", query.orderByColumn.name.c_str(), (query.orderByColumn.isDescending ? "DESC" : "ASC")) : "");
    auto select    = TextFormat("SELECT %s%s FROM %s%s%s LIMIT %d OFFSET %d;", distinct, columns, query.table.c_str(), filter.c_str(), orderBy.c_str(), query.limit, query.offset);
	auto statement = LSC_SQL::GetPreparedStatement(select);

	if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    if (hasWhere)
        sqlite3_bind_text(statement, 1, query.whereColumn.value.c_str(), -1, nullptr);

	auto rows = LSC_SQL::GetRows(statement);

	LSC_SQL::Finalize(statement);

	return rows;
}

void LSC_TableInsertRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto columnCount = columns.size();
    auto query       = TextFormat("INSERT INTO %s (", table.c_str());

    for (size_t i = 0; i < columnCount; i++)
    {
        if (!LSC_SQL::IsValid(columns[i].name))
            throw std::runtime_error(TextFormat("Invalid column name '%s'.", columns[i].name.c_str()));

        query.append(columns[i].name).append(i < (columnCount - 1) ? ", " : "");
    }
    
    query.append(") VALUES (");

    for (size_t i = 0; i < columnCount; i++)
        query.append("?").append(i < (columnCount - 1) ? ", " : "");

    query.append(");");

    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    for (size_t i = 0; i < columnCount; i++)
        sqlite3_bind_text(statement, (int)(i + 1), columns[i].value.c_str(), -1, nullptr);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);
}

bool LSC_TableRowExists(const std::string& table, const LSC_ColumnValue& whereColumn)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    if (whereColumn.name.empty() || !LSC_SQL::IsValid(whereColumn.name))
        throw std::runtime_error(TextFormat("Invalid where column name '%s'.", whereColumn.name.c_str()));

    auto select    = TextFormat("SELECT EXISTS(SELECT 1 FROM %s WHERE %s=?);", table.c_str(), whereColumn.name.c_str());
	auto statement = LSC_SQL::GetPreparedStatement(select);

	if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    sqlite3_bind_text(statement, 1, whereColumn.value.c_str(), -1, nullptr);

	bool exists = LSC_SQL::GetResultInt(statement);

	LSC_SQL::Finalize(statement);

	return exists;
}

void LSC_TableUpdateRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto columnCount = columns.size();
    auto query       = TextFormat("UPDATE %s SET ", table.c_str());

    for (size_t i = 0; i < columnCount; i++)
    {
        if (!LSC_SQL::IsValid(columns[i].name))
            throw std::runtime_error(TextFormat("Invalid column name '%s'.", columns[i].name.c_str()));

        query.append(TextFormat("%s=?", columns[i].name.c_str())).append(i < (columnCount - 1) ? ", " : "");
    }

    query.append(" WHERE id=?;");

    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    for (size_t i = 0; i < columnCount; i++)
        sqlite3_bind_text(statement, (int)(i + 1), columns[i].value.c_str(), -1, nullptr);

    sqlite3_bind_int64(statement, (int)(columnCount + 1), rowId);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);
}

#if defined _windows
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}
#else
int entry()
{
	return 0;
}
#endif
