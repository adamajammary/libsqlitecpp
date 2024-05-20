#include "LSC_Settings.h"

std::string LSC_Settings::Get(const std::string& key)
{
	auto query     = "SELECT value FROM lsc_settings WHERE key=?;";
	auto statement = LSC_SQL::GetPreparedStatement(query);

	if (!statement)
		throw std::runtime_error(TextFormat("Failed to get key: %s", key.c_str()));

	sqlite3_bind_text(statement, 1, key.c_str(), -1, nullptr);

	auto row = LSC_SQL::GetValue(statement);

	LSC_SQL::Finalize(statement);

	return row;
}

void LSC_Settings::Init()
{
	auto query = "CREATE TABLE IF NOT EXISTS lsc_settings (id INTEGER PRIMARY KEY, key TEXT UNIQUE NOT NULL, value TEXT NOT NULL);";

	if (LSC_SQL::Execute(query) != SQLITE_OK)
		throw std::runtime_error("Failed to create the settings table.");
}

void LSC_Settings::Set(const std::string& key, const std::string& value)
{
	auto query     = "INSERT INTO lsc_settings (key, value) VALUES (?,?) ON CONFLICT(key) DO UPDATE SET value=? WHERE key=?;";
	auto statement = LSC_SQL::GetPreparedStatement(query);

	if (!statement)
		throw std::runtime_error(TextFormat("Failed to set key '%s' with value '%s'.", key.c_str(), value.c_str()));

	sqlite3_bind_text(statement, 1, key.c_str(),   -1, nullptr);
	sqlite3_bind_text(statement, 2, value.c_str(), -1, nullptr);
	sqlite3_bind_text(statement, 3, value.c_str(), -1, nullptr);
	sqlite3_bind_text(statement, 4, key.c_str(),   -1, nullptr);

	auto result = LSC_SQL::Execute(statement);

	if (result != SQLITE_DONE)
		throw std::runtime_error(TextFormat("Failed to set key '%s' with value '%s'.", key.c_str(), value.c_str()));

	LSC_SQL::Finalize(statement);
}
