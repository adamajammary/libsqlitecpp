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

    bool isSearchable = false;

    auto query = TextFormat("CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY", table.c_str());

    for (const auto& column : columns)
    {
        if (!LSC_SQL::IsValid(column.name))
            throw std::runtime_error(TextFormat("Invalid column name '%s'.", column.name.c_str()));

        const char* type;

        switch (column.type) {
            case LSC_DATA_TYPE_FLOAT:   type = " REAL"; break;
            case LSC_DATA_TYPE_INTEGER: type = " INTEGER"; break;
            default: type = " TEXT COLLATE NOCASE"; break;
        }

        auto unique  = (column.isUnique  ? " UNIQUE"   : "");
        auto notNull = (column.isNotNull ? " NOT NULL" : "");

        query.append(TextFormat(", %s %s%s%s", column.name.c_str(), type, unique, notNull));

        if (column.isSearchable)
            isSearchable = true;
    }
    
    query.append(");");

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create table '%s'.", table.c_str()));

    if (!isSearchable)
        return;

    auto queryFTS = LSC_SQL::GetQueryFTS(table, columns);

    if (LSC_SQL::Execute(queryFTS) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create a virtual FTS table for '%s'.", table.c_str()));

    auto queryTriggerInsert = LSC_SQL::GetQueryTriggerInsert(table, columns);

    if (LSC_SQL::Execute(queryTriggerInsert) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create an insert trigger for '%s'.", table.c_str()));

    auto queryTriggerDelete = LSC_SQL::GetQueryTriggerDelete(table, columns);

    if (LSC_SQL::Execute(queryTriggerDelete) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create a delete trigger for '%s'.", table.c_str()));

    auto queryTriggerUpdate = LSC_SQL::GetQueryTriggerUpdate(table, columns);

    if (LSC_SQL::Execute(queryTriggerUpdate) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to create an update trigger for '%s'.", table.c_str()));
}

void LSC_TableDelete(const std::string& table)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

    auto query = TextFormat("DROP TABLE IF EXISTS %s;", table.c_str());

    if (LSC_SQL::Execute(query) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop table '%s'.", table.c_str()));

    auto queryFTS = TextFormat("DROP TABLE IF EXISTS %s_fts;", table.c_str());

    if (LSC_SQL::Execute(queryFTS) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop virtual FTS table for '%s'.", table.c_str()));

    auto queryTriggerInsert = TextFormat("DROP TRIGGER IF EXISTS %s_fts_insert;", table.c_str());

    if (LSC_SQL::Execute(queryTriggerInsert) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop insert trigger for '%s'.", table.c_str()));

    auto queryTriggerDelete = TextFormat("DROP TRIGGER IF EXISTS %s_fts_delete;", table.c_str());

    if (LSC_SQL::Execute(queryTriggerDelete) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop delete trigger for '%s'.", table.c_str()));

    auto queryTriggerUpdate = TextFormat("DROP TRIGGER IF EXISTS %s_fts_update;", table.c_str());

    if (LSC_SQL::Execute(queryTriggerUpdate) != SQLITE_OK)
        throw std::runtime_error(TextFormat("Failed to drop update trigger for '%s'.", table.c_str()));
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

int LSC_TableDeleteRows(const std::string& table, const LSC_WhereCondition& whereCondition)
{
    if (!LSC_SQL::IsValid(table))
        throw std::runtime_error(TextFormat("Invalid table name '%s'.", table.c_str()));

	auto whereClause = LSC_SQL::GetWhereCondition(whereCondition);
    auto query       = TextFormat("DELETE FROM %s WHERE %s;", table.c_str(), whereClause.c_str());
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

size_t LSC_TableGetRowCount(const LSC_Query& query)
{
    auto querySelect = LSC_SQL::GetSelect(query, true);
    auto select      = TextFormat("SELECT COUNT(*) FROM (%s);", querySelect.c_str());
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
        LSC_SQL::Bind(columns[i], (int)(i + 1), statement);

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

    LSC_SQL::Bind(whereColumn, 1, statement);

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
