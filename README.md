# libsqlitecpp

Copyright (C) 2024 Adam A. Jammary (Jammary Studio)

SQLite C++ Library

## 3rd Party Libraries

Library | Version | License
------- | ------- | -------
[SQLite](https://www.sqlite.org/) | [3.45.3](https://www.sqlite.org/2024/sqlite-autoconf-3450300.tar.gz) | [Public Domain](https://www.sqlite.org/copyright.html)

## Platform-dependent Include Headers

Platform | Header | Package
-------- | ------ | -------
Windows | windows.h | DllMain

## Compilers and C++20

libsqlitecpp uses modern [C++20](https://en.cppreference.com/w/cpp/compiler_support#cpp20) features and requires the following minimum compiler versions.

Compiler | Version
-------- | -------
CLANG | 14
GCC | 13
MSVC | 2019

## How to build

1. Build the [third-party libraries](#3rd-party-libraries) and place the them in a common directory.
1. Make sure you have [cmake](https://cmake.org/download/) installed.
1. Open a command prompt or terminal.
1. Create a **build** directory and enter it.
1. Run `cmake` to create a **Makefile**, **Xcode** project or **Visual Studio** solution based on your target platform.

```bash
mkdir build
cd build
```

### Android

Make sure you have [Android NDK](https://developer.android.com/ndk/downloads) installed.

```bash
cmake .. -G "Unix Makefiles" \
-D CMAKE_SYSTEM_NAME="Android" \
-D CMAKE_TOOLCHAIN_FILE="/path/to/ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-D ANDROID_NDK="/path/to/ANDROID_NDK" \
-D ANDROID_ABI="arm64-v8a" \
-D ANDROID_PLATFORM="android-29" \
-D EXT_LIB_DIR="/path/to/libs"

make
```

### iOS

You can get the iOS SDK path with the following command: `xcrun --sdk iphoneos --show-sdk-path`

```bash
/Applications/CMake.app/Contents/bin/cmake .. -G "Xcode" \
-D CMAKE_SYSTEM_NAME="iOS" \
-D CMAKE_OSX_ARCHITECTURES="arm64" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="12.5" \
-D CMAKE_OSX_SYSROOT="/path/to/IOS_SDK" \
-D IOS_SDK="iphoneos" \
-D EXT_LIB_DIR="/path/to/libs"

xcodebuild IPHONEOS_DEPLOYMENT_TARGET="12.5" -project sqlitecpp.xcodeproj -configuration Release
```

### macOS

You can get the macOS SDK path with the following command: `xcrun --sdk macosx --show-sdk-path`

```bash
/Applications/CMake.app/Contents/bin/cmake .. -G "Xcode" \
-D CMAKE_OSX_ARCHITECTURES="x86_64" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="12.6" \
-D CMAKE_OSX_SYSROOT="/path/to/MACOSX_SDK" \
-D EXT_LIB_DIR="/path/to/libs"

xcodebuild MACOSX_DEPLOYMENT_TARGET="12.6" -project sqlitecpp.xcodeproj -configuration Release
```

### Linux

```bash
cmake .. -G "Unix Makefiles" -D EXT_LIB_DIR="/path/to/libs"

make
```

### Windows

```bash
cmake .. -G "Visual Studio 17 2022" -D EXT_LIB_DIR="/path/to/libs"

devenv.com sqlitecpp.sln -build "Release|x64"
```

## Library

### LSC_ColumnDefinition

```cpp
struct LSC_ColumnDefinition
{
    std::string name = ""; // Required

    bool isNotNull    = false;
    bool isSearchable = false;
    bool isUnique     = false;
};
```

### LSC_ColumnOrderBy

```cpp
struct LSC_ColumnOrderBy
{
    std::string name = ""; // Required

    bool isDescending = false;
};
```

### LSC_ColumnValue

```cpp
struct LSC_ColumnValue
{
    std::string name  = ""; // Required
    std::string value = ""; // Required
};
```

### LSC_Query

```cpp
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
```

### LSC_TableRow

```cpp
using LSC_TableRow = std::unordered_map<std::string, std::string>;
```

### LSC_OpenDatabase

```cpp
void LSC_OpenDatabase(const std::string& filePath);
```

Opens or creates the database file, also creates a default key/value pair table used by LSC_Get and LSC_Set.

Parameters

- **filePath** Full writable UTF8 database file path

Throws **runtime_error**

```cpp
LSC_OpenDatabase("/path/to/my_database.db");
```

### LSC_CloseDatabase

```cpp
void LSC_CloseDatabase();
```

Cleans up allocated resources and closes the database connection.

### LSC_Get

```cpp
std::string LSC_Get(const std::string& key);
```

Returns the value from the default key/value pair table (if key exists).

Throws **runtime_error**

```cpp
std::string myValue = LSC_Get("my_key");
```

### LSC_Set

```cpp
void LSC_Set(const std::string& key, const std::string& value);
```

Adds key to the default key/value pair table.

Throws **runtime_error**

```cpp
LSC_Set("my_key", "my value");
```

### LSC_TableCreate

```cpp
void LSC_TableCreate(const std::string& table, const std::vector<LSC_ColumnDefinition>& columns);
```

Table and column names must start with a letter, and may only contain letters, numbers and underscores.

Throws **runtime_error**

```cpp
std::vector<LSC_ColumnDefinition> columns;

columns.push_back({ .name = "my_column1" });
columns.push_back({ .name = "my_column2", .isNotNull = true, .isSearchable = true, .isUnique = true });

LSC_TableCreate("my_table", columns);
```

### LSC_TableDelete

```cpp
void LSC_TableDelete(const std::string& table);
```

Throws **runtime_error**

### LSC_TableDeleteRow (ROW_ID)

```cpp
void LSC_TableDeleteRow(const std::string& table, int64_t rowId);
```

Deletes a row by primary key ID.

Throws **runtime_error**

```cpp
LSC_TableDeleteRow("my_table", 1);
```

### LSC_TableDeleteRow (COLUMN_VALUE)

```cpp
int LSC_TableDeleteRow(const std::string& table, const LSC_ColumnValue& column);
```

Deletes rows with a matching column value, and returns the number of rows deleted.

Throws **runtime_error**

```cpp
int rowsDeleted = LSC_TableDeleteRow("my_table", { .name = "my_column2", .value = "my value 2" });
```

### LSC_TableDeleteRows

Deletes all the rows in the table.

```cpp
void LSC_TableDeleteRows(const std::string& table);
```

Throws **runtime_error**

### LSC_TableGetRow

```cpp
LSC_TableRow LSC_TableGetRow(const std::string& table, int64_t rowId);
```

Selects a row by primary key ID.

Throws **runtime_error**

```cpp
LSC_TableRow row = LSC_TableGetRow("my_table", 1);
```

### LSC_TableGetRowCount

```cpp
size_t LSC_TableGetRowCount(const std::string& table);
```

Throws **runtime_error**

### LSC_TableGetRows

```cpp
std::vector<LSC_TableRow> LSC_TableGetRows(const LSC_Query& query);
```

Throws **runtime_error**

```cpp
// SELECT * FROM my_table LIMIT 100 OFFSET 0;

std::vector<LSC_TableRow> first100Rows = LSC_TableGetRows({ .table = "my_table" });
```

```cpp
// SELECT DISTINCT my_column1,my_column2 FROM my_table WHERE my_column1='my value 1' ORDER BY my_column2 DESC LIMIT 10 OFFSET 10;

LSC_Query query = {
    .table         = "my_table",
    .isDistinct    = true,
    .selectColumns = { "my_column1", "my_column2" },
    .whereColumn   = { .name = "my_column1", .value = "my value 1" },
    .orderByColumn = { .name = "my_column2", .isDescending = true },
    .limit         = 10,
    .offset        = 10
};

std::vector<LSC_TableRow> rows = LSC_TableGetRows(query);
```

### LSC_TableInsertRow

```cpp
void LSC_TableInsertRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns);
```

Throws **runtime_error**

```cpp
std::vector<LSC_ColumnValue> columns;

columns.push_back({ .name = "my_column1", .value = "my value 1" });
columns.push_back({ .name = "my_column2", .value = "my value 2" });

LSC_TableInsertRow("my_table", columns);
```

### LSC_TableRowExists

```cpp
bool LSC_TableRowExists(const std::string& table, const LSC_ColumnValue& whereColumn);
```

Returns true if a row with a matching column value already exists.

Throws **runtime_error**

```cpp
bool rowExists = LSC_TableRowExists("my_table", { .name = "my_column1", .value = "my value 1" });
```

### LSC_TableUpdateRow

```cpp
void LSC_TableUpdateRow(const std::string& table, const std::vector<LSC_ColumnValue>& columns, int64_t rowId);
```

Throws **runtime_error**

```cpp
std::vector<LSC_ColumnValue> columns;

columns.push_back({ .name = "my_column1", .value = "my new value 1" });
columns.push_back({ .name = "my_column2", .value = "my new value 2" });

LSC_TableUpdateRow("my_table", columns, 1);
```
