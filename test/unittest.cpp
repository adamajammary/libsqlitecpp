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

	TEST_CLASS(GetSet)
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
                columns.push_back({ .name = "test_column2", .isNotNull = true, .isSearchable = true, .isUnique = true });

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

                std::vector<LSC_ColumnValue> columns;

                columns.push_back({ .name = "test_column1", .value = "test;value1" });
                columns.push_back({ .name = "test_column2", .value = "test;value2" });

                LSC_TableInsertRow(TestTable, columns);

                columns.clear();

                columns.push_back({ .name = "test_column1", .value = "test;value1" });
                columns.push_back({ .name = "test_column2", .value = "test;value2A" });

                LSC_TableInsertRow(TestTable, columns);
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
                CreateTable();
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

                Assert::AreEqual(3, (int)row1.size());
                Assert::AreEqual(3, (int)row2.size());

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
                CreateTable();
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
                CreateTable();
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
                CreateTable();
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
                CreateTable();
                InsertRow();

                auto row1 = LSC_TableGetRow(TestTable, 1);
                auto row2 = LSC_TableGetRow(TestTable, 2);

                Assert::AreEqual(3, (int)row1.size());
                Assert::AreEqual(3, (int)row2.size());

                Assert::AreEqual("1",            row1["id"].c_str());
                Assert::AreEqual("test;value1",  row1["test_column1"].c_str());
                Assert::AreEqual("test;value2A", row2["test_column2"].c_str());
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
                CreateTable();
                InsertRow();

                auto rows = LSC_TableGetRows({ .table = TestTable });

                Assert::AreEqual(2, (int)rows.size());

                Assert::AreEqual("test;value1",  rows[0]["test_column1"].c_str());
                Assert::AreEqual("test;value2A", rows[1]["test_column2"].c_str());
            }
            catch (const std::exception& e)
            {
                Assert::Fail(ToString(e.what()).c_str());
            }
        }

        TEST_METHOD(GetRows)
        {
            try
            {
                CreateTable();
                InsertRow();

                LSC_Query query = {
                    .table         = TestTable,
                    .isDistinct    = true,
                    .selectColumns = { "test_column2" },
                    .whereColumn   = { .name = "test_column2", .value = "test;value2A" },
                    .orderByColumn = { .name = "test_column2", .isDescending = true },
                    .limit         = 1,
                    .offset        = 0
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
    };
}

#endif
