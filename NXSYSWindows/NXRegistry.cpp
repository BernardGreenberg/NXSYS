#include <windows.h>
#include <string>
#include <vector>
#include "NXRegistry.h"

using std::string;

static const char* NXSYS_REG_KEY = "SOFTWARE\\B.Greenberg\\NXSYS";


HKEY GetAppKey(LPCSTR subk) {
	HKEY hk;
	string path{ NXSYS_REG_KEY };
	path += "\\";
	path += subk;
	if (ERROR_SUCCESS ==
		RegCreateKeyEx
		(HKEY_CURRENT_USER,
			path.c_str(),
			0,
			NULL,
			0,
			KEY_ALL_ACCESS,
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

DWORDRegValue getDWORDRegVal(HKEY key, const string& name) {
	if (key) {
		DWORD val;
		DWORD bufsize = sizeof(val);
		DWORD type;
		DWORD ec = RegQueryValueEx(key, name.c_str(), NULL,
			&type, (PBYTE)&val, &bufsize);
		if (ec == ERROR_SUCCESS)
			return DWORDRegValue(val);
	}
	return DWORDRegValue();
}

DWORD PutDWORDRegval(HKEY key, LPCSTR vname, DWORD value) {
	if (key)
		RegSetValueEx(key, vname, 0,
			REG_DWORD, (LPBYTE)&value, sizeof(value));
	return value;
}

StringRegValue getStringRegval(HKEY key, const std::string& vname, size_t size) {
	std::vector<char> buff(size);
	DWORD bufsize = (DWORD)size;
	DWORD type;
	DWORD ec = RegQueryValueEx(key, vname.c_str(), NULL, &type, (PBYTE)buff.data(), &bufsize);
	if (ec == ERROR_SUCCESS) {
		/* Bombs lay in trailing zeros! */
		while (bufsize > 0 && buff[bufsize - 1] == 0)
			bufsize -= 1;
		return StringRegValue(string(buff.data(), bufsize));
	}
	return StringRegValue();
}

bool putStringRegval(HKEY key, const std::string& vname, const std::string& value) {
	DWORD ec = RegSetValueEx(key, vname.c_str(), 0, REG_SZ, (PBYTE)value.c_str(), (DWORD)value.length()+1);
	return (ec == ERROR_SUCCESS);
}
