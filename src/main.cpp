#include "main.h"

sqlite3* connection = nullptr;

sqlite3* LSC_GetDBConnection()
{
    return connection;
}

void LSC_Clear()
{
    LSC_Settings::Clear();
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
        throw std::runtime_error(std::format("Failed to open/write: {}", filePath));

    LSC_Settings::Init();
}

void LSC_Set(const std::string& key, const std::string& value)
{
    LSC_Settings::Set(key, value);
}

void LSC_TableCreate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    bool isSearchable = false;

    auto query = std::format("CREATE TABLE IF NOT EXISTS {} (id INTEGER PRIMARY KEY", table);

    for (const auto& column : columns)
    {
        if (!LSC_SQL::IsValid(column.name))
            throw std::runtime_error(std::format("Invalid column name '{}'.", column.name));

        const char* type;

        switch (column.type) {
            case LSC_DATA_TYPE_FLOAT:   type = " REAL"; break;
            case LSC_DATA_TYPE_INTEGER: type = " INTEGER"; break;
            default: type = " TEXT COLLATE NOCASE"; break;
        }

        auto unique  = (column.isUnique  ? " UNIQUE"   : "");
        auto notNull = (column.isNotNull ? " NOT NULL" : "");

        query.append(std::format(", {} {}{}{}", column.name, type, unique, notNull));

        if (column.isSearchable)
            isSearchable = true;
    }
    
    query.append(");");

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create table '{}'.", table));

    if (!isSearchable)
        return;

    auto queryFTS = LSC_SQL::GetQueryFTS(table, columns);

    if (LSC_SQL::Execute(queryFTS) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create a virtual FTS table for '{}'.", table));

    auto queryTriggerInsert = LSC_SQL::GetQueryTriggerInsert(table, columns);

    if (LSC_SQL::Execute(queryTriggerInsert) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create an insert trigger for '{}'.", table));

    auto queryTriggerDelete = LSC_SQL::GetQueryTriggerDelete(table, columns);

    if (LSC_SQL::Execute(queryTriggerDelete) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create a delete trigger for '{}'.", table));

    auto queryTriggerUpdate = LSC_SQL::GetQueryTriggerUpdate(table, columns);

    if (LSC_SQL::Execute(queryTriggerUpdate) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create an update trigger for '{}'.", table));
}

void LSC_TableCreateIndex(const std::string& table, const std::string& index, const std::vector<LSC_ColumnOrderBy>& columns)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    if (!LSC_SQL::IsValid(index))
        throw std::runtime_error(std::format("Invalid index name '{}'.", index));

    auto columnCount = columns.size();
    auto query       = std::format("CREATE INDEX IF NOT EXISTS {} ON {} (", index, table);

    for (size_t i = 0; i < columnCount; i++)
    {
        if (!LSC_SQL::IsValid(columns[i].name))
            throw std::runtime_error(std::format("Invalid column name '{}'.", columns[i].name));

        query.append(std::format("{} {}", columns[i].name, (columns[i].isDescending ? "DESC" : "ASC")));

        if (i < (columnCount - 1))
            query.append(", ");
    }

    query.append(");");

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to create index '{}'.", index));
}

void LSC_TableDelete(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto query = std::format("DROP TABLE IF EXISTS {};", table);

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop table '{}'.", table));

    auto queryFTS = std::format("DROP TABLE IF EXISTS {}_fts;", table);

    if (LSC_SQL::Execute(queryFTS) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop virtual FTS table for '{}'.", table));

    auto queryTriggerInsert = std::format("DROP TRIGGER IF EXISTS {}_fts_insert;", table);

    if (LSC_SQL::Execute(queryTriggerInsert) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop insert trigger for '{}'.", table));

    auto queryTriggerDelete = std::format("DROP TRIGGER IF EXISTS {}_fts_delete;", table);

    if (LSC_SQL::Execute(queryTriggerDelete) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop delete trigger for '{}'.", table));

    auto queryTriggerUpdate = std::format("DROP TRIGGER IF EXISTS {}_fts_update;", table);

    if (LSC_SQL::Execute(queryTriggerUpdate) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop update trigger for '{}'.", table));
}

void LSC_TableDeleteIndex(const std::string& index)
{
    if (!LSC_SQL::IsValid(index))
        throw std::runtime_error(std::format("Invalid index name '{}'.", index));

    auto query = std::format("DROP INDEX IF EXISTS {};", index);

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to drop index '{}'.", index));
}

void LSC_TableDeleteRow(const std::string& table, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto query     = std::format("DELETE FROM {} WHERE id=?;", table);
    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    sqlite3_bind_int64(statement, 1, rowId);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);
}

int LSC_TableDeleteRows(const std::string& table, const LSC_WhereCondition& whereCondition)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

	auto whereClause = LSC_SQL::GetWhereCondition(whereCondition);
    auto query       = std::format("DELETE FROM {} WHERE {};", table, whereClause);
    auto statement   = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    for (int i = 0; i < (int)whereCondition.columns.size(); i++)
        LSC_SQL::Bind(whereCondition.columns[i], (i + 1), statement);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);

    return sqlite3_changes(connection);
}

void LSC_TableDeleteRows(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto query = std::format("DELETE FROM {};", table);

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(std::format("Failed to truncate table '{}'.", table));
}

LSC_TableRow LSC_TableGetRow(const std::string& table, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto query     = std::format("SELECT * FROM {} WHERE id=?;", table);
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
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto select    = std::format("SELECT COUNT(*) FROM {};", table);
	auto statement = LSC_SQL::GetPreparedStatement(select);

	if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

	auto count = (size_t)LSC_SQL::GetResultInt(statement);

	LSC_SQL::Finalize(statement);

	return count;
}

size_t LSC_TableGetRowCount(const LSC_Query& query)
{
    auto querySelect = LSC_SQL::GetSelect(query, true);
    auto select      = std::format("SELECT COUNT(*) FROM ({});", querySelect);
    auto statement   = LSC_SQL::GetPreparedStatement(select);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    LSC_SQL::Bind(query, statement);

    auto count = (size_t)LSC_SQL::GetResultInt(statement);

    LSC_SQL::Finalize(statement);

    return count;
}

LSC_TableRows LSC_TableGetRows(const LSC_Query& query)
{
    auto select    = LSC_SQL::GetSelect(query);
    auto statement = LSC_SQL::GetPreparedStatement(select);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    LSC_SQL::Bind(query, statement);

    auto rows = LSC_SQL::GetRows(statement);

    LSC_SQL::Finalize(statement);

    return rows;
}

void LSC_TableInsertRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto columnCount = columns.size();
    auto query       = std::format("INSERT INTO {} (", table);

    for (size_t i = 0; i < columnCount; i++)
    {
        if (!LSC_SQL::IsValid(columns[i].name))
            throw std::runtime_error(std::format("Invalid column name '{}'.", columns[i].name));

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
        LSC_SQL::Bind(columns[i], (int)(i + 1), statement);

    auto result = LSC_SQL::Execute(statement);

    if (result != SQLITE_DONE)
        throw std::runtime_error("Failed to execute the statement.");

    LSC_SQL::Finalize(statement);
}

bool LSC_TableRowExists(const std::string& table, const LSC_ColumnValue& whereColumn)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    if (whereColumn.name.empty() || !LSC_SQL::IsValid(whereColumn.name))
        throw std::runtime_error(std::format("Invalid where column name '{}'.", whereColumn.name));

    auto select    = std::format("SELECT EXISTS(SELECT 1 FROM {} WHERE {}=?);", table, whereColumn.name);
	auto statement = LSC_SQL::GetPreparedStatement(select);

	if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    LSC_SQL::Bind(whereColumn, 1, statement);

	bool exists = LSC_SQL::GetResultInt(statement);

	LSC_SQL::Finalize(statement);

	return exists;
}

void LSC_TableUpdateRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns, int64_t rowId)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(std::format("Invalid table name '{}'.", table));

    auto columnCount = columns.size();
    auto query       = std::format("UPDATE {} SET ", table);

    for (size_t i = 0; i < columnCount; i++)
    {
        if (!LSC_SQL::IsValid(columns[i].name))
            throw std::runtime_error(std::format("Invalid column name '{}'.", columns[i].name));

        query.append(std::format("{}=?", columns[i].name)).append(i < (columnCount - 1) ? ", " : "");
    }

    query.append(" WHERE id=?;");

    auto statement = LSC_SQL::GetPreparedStatement(query);

    if (!statement)
        throw std::runtime_error("Failed to prepare the statement.");

    for (size_t i = 0; i < columnCount; i++)
        LSC_SQL::Bind(columns[i], (int)(i + 1), statement);

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
