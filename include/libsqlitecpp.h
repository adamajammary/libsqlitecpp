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

using LSC_TableRow = std::unordered_map<std::string, std::string>;

struct LSC_ColumnDefinition
{
    std::string name = ""; // Required

    bool isNotNull    = false;
    bool isSearchable = false;
    bool isUnique     = false;
};

struct LSC_ColumnOrderBy
{
    std::string name = ""; // Required

    bool isDescending = false;
};

struct LSC_ColumnValue
{
    std::string name  = ""; // Required
    std::string value = ""; // Required
};

struct LSC_Query
{
    std::string table = ""; // Required

    bool isDistinct = false;

    std::vector<std::string> selectColumns;

    LSC_ColumnValue whereColumn;

    LSC_ColumnOrderBy orderByColumn;

    int limit  = 100;
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
 * @brief Selects a row by primary key ID.
 * @throws runtime_error
 */
DLLEXPORT LSC_TableRow DLL LSC_TableGetRow(const std::string& table, int64_t rowId);

/**
 * @throws runtime_error
 */
DLLEXPORT std::vector<LSC_TableRow> DLL LSC_TableGetRows(const LSC_Query& query);

/**
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableInsertRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns);

/**
 * @throws runtime_error
 */
DLLEXPORT void DLL LSC_TableUpdateRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns, int64_t rowId);

#endif
