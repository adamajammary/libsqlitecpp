#ifndef LIBSQLITECPP_H
#define LIBSQLITECPP_H

#if defined _windows
    #define DLL __cdecl

    #ifdef sqlitecpp_EXPORTS
	    #define DLLEXPORT __declspec(dllexport)
    #else
	    #define DLLEXPORT __declspec(dllimport)
    #endif
#else
    #define DLL

    #if __GNUC__ >= 4
        #define DLLEXPORT __attribute__ ((visibility ("default")))
    #else
        #define DLLEXPORT
    #endif
#endif

#include <string>
#include <vector>
#include <unordered_map>

using LSC_TableRow  = std::unordered_map<std::string, std::string>;
using LSC_TableRows = std::vector<LSC_TableRow>;

enum LSC_Comparison
{
    LSC_COMPARISON_EQUALS,
    LSC_COMPARISON_NOT_EQUALS,
    LSC_COMPARISON_IS_NULL,
    LSC_COMPARISON_IS_NOT_NULL,
    LSC_COMPARISON_GREATER_THAN,
    LSC_COMPARISON_GREATER_THAN_OR_EQUALS,
    LSC_COMPARISON_LESS_THAN,
    LSC_COMPARISON_LESS_THAN_OR_EQUALS
};

enum LSC_DataType
{
    LSC_DATA_TYPE_FLOAT,
    LSC_DATA_TYPE_INTEGER,
    LSC_DATA_TYPE_TEXT
};

enum LSC_Operation
{
    LSC_OPERATION_AND,
    LSC_OPERATION_OR
};

struct LSC_ColumnDefinition
{
    std::string name = ""; // Required

    LSC_DataType type = LSC_DATA_TYPE_TEXT;

    bool isUnique     = false;
    bool isNotNull    = false;
    bool isSearchable = false;
};

struct LSC_ColumnOrderBy
{
    std::string name = ""; // Required

    bool isDescending = false;
};

struct LSC_ColumnValue
{
    std::string name  = ""; // Required
    std::string value = "";

    LSC_Comparison comparison = LSC_COMPARISON_EQUALS;

    LSC_DataType type = LSC_DATA_TYPE_TEXT;
};

struct LSC_WhereCondition
{
    std::vector<LSC_ColumnValue> columns;

    LSC_Operation operation = LSC_OPERATION_AND;
};

struct LSC_Query
{
    std::string table = ""; // Required

    bool isDistinct = false;

    std::vector<std::string> selectColumns;

    LSC_WhereCondition whereCondition;

    std::string search = "";

    LSC_ColumnOrderBy orderByColumn;

    int limit  = 100000;
    int offset = 0;
};

/**
 * @brief Opens or creates the database file, also creates a default key/value pair table used by LSC_Get and LSC_Set.
 * @param filePath Full writable UTF8 database file path
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_OpenDatabase(const std::string& filePath);

/**
 * @brief Cleans up allocated resources and closes the database connection.
 */
DLLEXPORT void DLL LSC_CloseDatabase();

/**
 * @returns the value from the default key/value pair table (if key exists)
 * @throws runtime_error
 */
DLLEXPORT std::string DLL LSC_Get(const std::string& key);

/**
 * @brief Adds key to the default key/value pair table.
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_Set(const std::string& key, const std::string& value);

/**
 * @brief Table and column names must start with a letter, and may only contain letters, numbers and underscores.
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableCreate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns);

/**
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableDelete(const std::string& table);

/**
 * @brief Deletes a row by primary key ID.
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableDeleteRow(const std::string& table, int64_t rowId);

/**
 * @brief Deletes rows with a matching column value.
 * @returns the number of rows deleted
 * @throws runtime_error
 */
DLLEXPORT int DLL LSC_TableDeleteRow(const std::string& table, const LSC_ColumnValue& column);

/**
 * Deletes all the rows in the table.
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableDeleteRows(const std::string& table);

/**
 * @brief Selects a row by primary key ID.
 * @throws runtime_error
 */
DLLEXPORT LSC_TableRow DLL LSC_TableGetRow(const std::string& table, int64_t rowId);

/**
 * @returns the number of rows in the table
 * @throws runtime_error
 */
DLLEXPORT size_t DLL LSC_TableGetRowCount(const std::string& table);

/**
 * @returns the number of rows the query would return
 * @throws runtime_error
 */
DLLEXPORT size_t DLL LSC_TableGetRowCount(const LSC_Query& query);

/**
 * @throws runtime_error
 */
DLLEXPORT LSC_TableRows DLL LSC_TableGetRows(const LSC_Query& query);

/**
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableInsertRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns);

/**
 * @returns true if a row with a matching column value already exists
 * @throws runtime_error
 */
DLLEXPORT bool DLL LSC_TableRowExists(const std::string& table, const LSC_ColumnValue& whereColumn);

/**
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableUpdateRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns, int64_t rowId);

#endif
