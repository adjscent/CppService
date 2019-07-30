#pragma once
#include <cpr/cpr.h>
#include <Lmcons.h>
#include <comdef.h>
#include <stdio.h>
#include <psapi.h>
#include <windows.h>

#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>
class EnumProcess
{
public:
	EnumProcess();
	~EnumProcess();
	static std::set<std::string> EnumProcessForUser();
};

