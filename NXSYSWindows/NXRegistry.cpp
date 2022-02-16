#include <windows.h>
#include <string>
#include <vector>
#include "NXRegistry.h"

using std::string;

#define NXSYS_LM_REG_KEY "SOFTWARE\\B.Greenberg\\NXSYS"

static constexpr UINT SAM = KEY_WOW64_64KEY;

HKEY GetAppKey(LPCSTR subk) {
	HKEY hk;
	string path{ NXSYS_LM_REG_KEY };
	path += "\\";
	path += subk;
	if (ERROR_SUCCESS ==
		RegCreateKeyEx
		(HKEY_CURRENT_USER,
			path.c_str(),
			0,
			NULL,
			0,
			KEY_ALL_ACCESS | SAM,
			NULL,
			&hk,
			NULL))
		return hk;
	else
		return NULL;
}

DWORD GetDWORDRegval(HKEY key, LPCSTR vname, DWORD val) {
	if (key) {
		DWORD bufsize = sizeof(val);
		DWORD type;
		RegQueryValueEx(key, vname, NULL,
			&type, (PBYTE)&val, &bufsize);
	}
	return val;
}

DWORD PutDWORDRegval(HKEY key, LPCSTR vname, DWORD value) {
	if (key)
		RegSetValueEx(key, vname, 0,
			REG_DWORD, (LPBYTE)&value, sizeof(value));
	return value;
}

StringRegValue getStringRegItem(const char* name, size_t size) {
	HKEY hKey = 0;
	std::vector<char> buff(size);
	DWORD bufsize = size;
	DWORD type;
	DWORD ec = RegOpenKeyEx(HKEY_LOCAL_MACHINE, NXSYS_LM_REG_KEY, 0, KEY_READ | SAM, &hKey);
	if (ec == ERROR_SUCCESS) {
		ec = RegQueryValueEx(hKey, name, NULL, &type, (PBYTE)buff.data(), &bufsize);
		RegCloseKey(hKey);
		if (ec == ERROR_SUCCESS)
			return StringRegValue(string(buff.data(), bufsize));
	}
	return StringRegValue();
}
