#pragma once
#include <windows.h>
#include <string>

struct StringRegValue {
	std::string value;
	bool valid;
	StringRegValue(const std::string s) : valid(true), value(s) {}
	StringRegValue() : valid(false), value("") {}
};


HKEY GetAppKey(LPCSTR subk);
DWORD GetDWORDRegval(HKEY key, LPCSTR vname, DWORD val);
StringRegValue getStringRegItem(const char* name, size_t size = MAX_PATH);