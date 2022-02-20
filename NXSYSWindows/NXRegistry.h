#pragma once
#include <windows.h>
#include <string>

#define SOURCE_LOCATOR_SCRIPT_VALUE_NAME "SourceLocatorScript"

struct StringRegValue {
	std::string value;
	bool valid;
	StringRegValue(const std::string s) : valid(true), value(s) {}
	StringRegValue() : valid(false), value("") {}
};

struct DWORDRegValue {
	bool valid;
	DWORD value;
	DWORDRegValue(DWORD d) : valid(true), value(d) {}
	DWORDRegValue() : valid(false), value(0) {}
};

HKEY GetAppKey(LPCSTR subk);
DWORD GetDWORDRegval(HKEY key, LPCSTR vname, DWORD val);
StringRegValue getStringRegval(HKEY key, const std::string& name, size_t size=MAX_PATH);
DWORDRegValue getDWORDRegVal(HKEY key, const std::string& name);
bool putStringRegval(HKEY key, const std::string& name, const std::string& value);
DWORD PutDWORDRegval(HKEY key, LPCSTR vname, DWORD value);

struct AppKey {
	HKEY hKey;
	AppKey(const char* subk) {
		hKey = GetAppKey(subk);
	}
	~AppKey() { if (hKey) RegCloseKey(hKey); }
	operator bool() { return (hKey != 0); }
	operator HKEY() { return hKey; }
};