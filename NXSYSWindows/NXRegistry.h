#pragma once
#include <windows.h>
#include <string>
#include "ValidatingValue.h"

#define SOURCE_LOCATOR_SCRIPT_VALUE_NAME "SourceLocatorScript"

HKEY GetAppKey(LPCSTR subk);
DWORD GetDWORDRegval(HKEY key, LPCSTR vname, DWORD val);
ValidatingValue<std::string> getStringRegval(HKEY key, const std::string& name, size_t size=MAX_PATH);
ValidatingValue<DWORD> getDWORDRegVal(HKEY key, const std::string& name);
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