#include "main.h"

#ifndef LSC_SETTINGS_H
#define LSC_SETTINGS_H

class LSC_Settings
{
private:
	LSC_Settings()  {}
	~LSC_Settings() {}

public:
	static std::string Get(const std::string& key);
	static void        Init();
	static void        Set(const std::string& key, const std::string& value);
};

#endif
