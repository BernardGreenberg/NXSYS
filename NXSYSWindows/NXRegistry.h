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


HKEY GetAppKey(LPCSTR subk);
DWORD GetDWORDRegval(HKEY key, LPCSTR vname, DWORD val);
StringRegValue getStringRegval(HKEY key, const std::string& name, size_t size=MAX_PATH);
bool putStringRegval(HKEY key, const std::string& name, const std::string& value);

struct AppKey {
	HKEY hKey;
	AppKey(const char* subk) {
		hKey = GetAppKey(subk);
	}
	~AppKey() { if (hKey) RegCloseKey(hKey); }
	operator bool() { return (hKey != 0); }
	operator HKEY() { return hKey; }
};