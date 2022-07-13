#include <windows.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "compat32.h"
#include "resource.h"
#include <string>
#include <vector>
#include <winver.h>
#include "getmodtm.h"
#include "Resources2022.h"
#include "WinApiSTL.h"
#include "STLExtensions.h"
#include "AppBuildSignature.h"

using std::string;

string AppBuildSignature::OSBase() {
#ifdef _WIN64
	return "Windows (64 bit)";
#else
	return "Windows (32 bit)";
#endif
}

void AppBuildSignature::Populate() {
	BuildTime = GetModuleTime(NULL);
	char Path[_MAX_PATH]{};
	GetModuleFileName(NULL, Path, _MAX_PATH);

	size_t version_data_len = GetFileVersionInfoSize(Path, NULL);
	std::vector<char> infov(version_data_len);

	LPVOID vdp = &infov[0];
	GetFileVersionInfo(Path, NULL, (DWORD)version_data_len, vdp);
	VS_FIXEDFILEINFO* pFileInfo = NULL;
	UINT puLenFileInfo = 0;
	VerQueryValue(vdp, TEXT("\\"), (LPVOID*)&pFileInfo, &puLenFileInfo);
	VersionComponents[0] = HIWORD(pFileInfo->dwFileVersionMS);
	VersionComponents[1] = LOWORD(pFileInfo->dwFileVersionMS);
	VersionComponents[2] = HIWORD(pFileInfo->dwFileVersionLS);
	VersionComponents[3] = LOWORD(pFileInfo->dwFileVersionLS);

	UINT cbLang{};
	WORD* langInfo = nullptr;;

	VerQueryValue(vdp, "\\VarFileInfo\\Translation", (LPVOID*)&langInfo, &cbLang);
	//Prepare the label -- default lang is bytes 0 & 1 of langInfo
	string key = FormatString("\\StringFileInfo\\%04x%04x\\%s",
              langInfo[0], langInfo[1], "InternalName");
	//Get the string from the resource data
	LPTSTR lpProdName = nullptr;
	UINT cbBufSize{};
	if (VerQueryValue(vdp, key.c_str(), (LPVOID*)&lpProdName, &cbBufSize))
		ApplicationName = lpProdName;
}
