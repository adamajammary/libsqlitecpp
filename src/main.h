#ifndef LSC_MAIN_H
#define LSC_MAIN_H

#include <regex>
#include <stdexcept>

#if defined _windows
	#include <windows.h> // DllMain(x)
#endif

extern "C"
{
	#include <sqlite3.h>
}

#include <libsqlitecpp.h>

template<typename... Args>
static std::string TextFormat(const char* formatString, const Args&... args)
{
    if (!formatString)
        return "";

    char buffer[1024] = {};
    std::snprintf(buffer, 1024, formatString, args...);

    return std::string(buffer);
}

sqlite3* LSC_GetDBConnection();

#include "LSC_Settings.h"
#include "LSC_SQL.h"

#endif
