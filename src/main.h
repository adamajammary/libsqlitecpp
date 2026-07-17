#ifndef LSC_MAIN_H
#define LSC_MAIN_H

#include <cstring> // strcmp()
#include <format>
#include <regex>
#include <stdexcept>

#if defined _windows
	#include <windows.h> // DllMain()
#endif

extern "C"
{
	#include <sqlite3.h>
}

#include <libsqlitecpp.h>

sqlite3* LSC_GetDBConnection();

#include "LSC_Settings.h"
#include "LSC_SQL.h"

#endif
