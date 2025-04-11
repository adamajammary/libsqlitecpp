#if defined _windows && defined _DEBUG

#include <windows.h>
#include <CppUnitTest.h>
#include <libsqlitecpp.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LSC_UnitTest
{
    const std::string TestTable = "Test_Table1";

    TEST_MODULE_INITIALIZE(Start)
    {
        try
        {
            HMODULE      hmodule;
            static TCHAR moduleName;
            TCHAR        pathA[MAX_PATH + 1];

            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, &moduleName, &hmodule);
            GetModuleFileNameA(hmodule, pathA, (MAX_PATH + 1));

            LSC_OpenDatabase(std::string(pathA).append(".test.db"));
        }
        catch (const std::exception& e)
        {
            Assert::Fail(ToString(e.what()).c_str());
        }
    }

    TEST_MODULE_CLEANUP(Quit)
    {
        LSC_CloseDatabase();
    }

	TEST_CLASS(Settings)
	{
        TEST_METHOD(Set)
        {
            try
            {
                LSC_Set("test;key", "test;value");
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(Get)
        {
            try
            {
                Set();

                auto value1 = LSC_Get("test;key");

                Assert::IsFalse(value1.empty());
                Assert::AreEqual("test;value", value1.c_str());

                auto value2 = LSC_Get("test_key");

                Assert::IsTrue(value2.empty());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }
	};

	TEST_CLASS(Table)
	{
        TEST_METHOD(CreateTable)
        {
            try
            {
                LSC_TableDelete(TestTable);

                std::vector<LSC_ColumnDefinition> columns;

                columns.push_back({ .name = "test_column1" });
                columns.push_back({ .name = "test_column2", .isUnique = true, .isNotNull = true, .isSearchable = true });
                columns.push_back({ .name = "test_column_float",   .type = LSC_DATA_TYPE_FLOAT });
                columns.push_back({ .name = "test_column_integer", .type = LSC_DATA_TYPE_INTEGER });

                LSC_TableCreate(TestTable, columns);
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(InsertRow)
        {
            try
            {
                CreateTable();

                std::vector<LSC_ColumnValue> row1 = {
                    { .name = "test_column1", .value = "test;value1" },
                    { .name = "test_column2", .value = "test;value2" },
                    { .name = "test_column_float",   .value = "15.1" },
                    { .name = "test_column_integer", .value = "15" },
                };

                LSC_TableInsertRow(TestTable, row1);

                std::vector<LSC_ColumnValue> row2 = {
                    {.name = "test_column1", .value = "test;value1" },
                    {.name = "test_column2", .value = "test;value2A" },
                    {.name = "test_column_float",   .value = "1.5" },
                    {.name = "test_column_integer", .value = "1" }
                };

                LSC_TableInsertRow(TestTable, row2);
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(UpdateRow)
        {
            try
            {
                InsertRow();

                std::vector<LSC_ColumnValue> columns;

                columns.push_back({ .name = "test_column1", .value = "new;test;value1" });
                columns.push_back({ .name = "test_column2", .value = "new;test;value2" });

                LSC_TableUpdateRow(TestTable, columns, 1);

                columns.clear();

                columns.push_back({ .name = "test_column1", .value = "new;test;value1" });
                columns.push_back({ .name = "test_column2", .value = "new;test;value2A" });

                LSC_TableUpdateRow(TestTable, columns, 2);

                auto row1 = LSC_TableGetRow(TestTable, 1);
                auto row2 = LSC_TableGetRow(TestTable, 2);

                Assert::AreEqual(5, (int)row1.size());
                Assert::AreEqual(5, (int)row2.size());

                Assert::AreEqual("new;test;value1",  row1["test_column1"].c_str());
                Assert::AreEqual("new;test;value2A", row2["test_column2"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(DeleteRowById)
        {
            try
            {
                InsertRow();

                LSC_TableDeleteRow(TestTable, 1);

                auto row1 = LSC_TableGetRow(TestTable, 1);

                Assert::IsTrue(row1.empty());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(DeleteRowByValue)
        {
            try
            {
                InsertRow();

                auto rowsDeleted = LSC_TableDeleteRow(TestTable, { .name = "test_column1", .value = "test;value1" });

                Assert::AreEqual(2, rowsDeleted);
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(DeleteRows)
        {
            try
            {
                InsertRow();

                LSC_TableDeleteRows(TestTable);

                auto rows = LSC_TableGetRows({ .table = TestTable });

                Assert::IsTrue(rows.empty());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(GetRow)
        {
            try
            {
                InsertRow();

                auto row = LSC_TableGetRow(TestTable, 1);

                Assert::AreEqual(5, (int)row.size());

                Assert::AreEqual("1",           row["id"].c_str());
                Assert::AreEqual("test;value1", row["test_column1"].c_str());
                Assert::AreEqual("test;value2", row["test_column2"].c_str());
                Assert::AreEqual("15",          row["test_column_integer"].c_str());
                Assert::AreEqual("15.1",        row["test_column_float"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(GetRowsAll)
        {
            try
            {
                InsertRow();

                auto rows = LSC_TableGetRows({ .table = TestTable });

                Assert::AreEqual(2, (int)rows.size());

                Assert::AreEqual("test;value1",  rows[0]["test_column1"].c_str());
                Assert::AreEqual("test;value2A", rows[1]["test_column2"].c_str());

                rows = LSC_TableGetRows({ .table = TestTable, .orderByColumns = {{ .name = "test_column_integer" }} });

                Assert::AreEqual(2, (int)rows.size());

                Assert::AreEqual("1",  rows[0]["test_column_integer"].c_str());
                Assert::AreEqual("15", rows[1]["test_column_integer"].c_str());

                rows = LSC_TableGetRows({ .table = TestTable, .orderByColumns = {{ .name = "test_column_float" }} });

                Assert::AreEqual(2, (int)rows.size());

                Assert::AreEqual("1.5",  rows[0]["test_column_float"].c_str());
                Assert::AreEqual("15.1", rows[1]["test_column_float"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(GetRowsWhere)
        {
            try
            {
                InsertRow();

                LSC_Query query = {
                    .table          = TestTable,
                    .isDistinct     = true,
                    .selectColumns  = { "test_column2" },
                    .whereCondition = { .columns = {{ .name = "test_column2", .value = "test;value2A" }} },
                    .orderByColumns = {{ .name = "test_column2", .isDescending = true }},
                    .limit          = 1,
                    .offset         = 0
                };

                auto rows = LSC_TableGetRows(query);

                Assert::AreEqual(1, (int)rows.size());

                Assert::AreEqual("test;value2A", rows[0]["test_column2"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(GetRowsSearch)
        {
            try
            {
                InsertRow();

                LSC_Query query = {
                    .table          = TestTable,
                    .isDistinct     = true,
                    .selectColumns  = { "test_column2" },
                    .whereCondition = { .columns = {{ .name = "test_column2", .value = "test;value2A" }} },
                    .search         = "value2",
                    .orderByColumns = {{ .name = "test_column2", .isDescending = true }},
                    .limit          = 1,
                    .offset         = 0
                };

                auto rows = LSC_TableGetRows(query);

                Assert::AreEqual(1, (int)rows.size());

                Assert::AreEqual("test;value2A", rows[0]["test_column2"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(RowCount)
        {
            try
            {
                InsertRow();

                auto tableCount = LSC_TableGetRowCount(TestTable);

                Assert::AreEqual(2, (int)tableCount);

                LSC_Query query = {
                    .table          = TestTable,
                    .selectColumns  = { "test_column2" },
                    .whereCondition = { .columns = {{ .name = "test_column2", .value = "test;value2A" }} }
                };

                auto queryCount = LSC_TableGetRowCount(query);

                Assert::AreEqual(1, (int)queryCount);
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(RowExists)
        {
            try
            {
                InsertRow();

                auto exists1 = LSC_TableRowExists(TestTable, { .name = "test_column1", .value = "test;value1" });
                auto exists2 = LSC_TableRowExists(TestTable, { .name = "test_column1", .value = "test;value3" });

                Assert::IsTrue(exists1);
                Assert::IsFalse(exists2);
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }
    };
}

#endif
