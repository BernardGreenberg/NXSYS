#include <vector>
#include <string>
#include <stdlib.h>
#include <windows.h>
#include "replace_filename.h"
#include "WinReadResText.h"

using std::string, std::vector;

string GMFN() {
	vector<char> mpath(1024);
	string expanded_path;
	GetModuleFileName(NULL, &mpath[0], (DWORD)mpath.size());
	return string(mpath.data());
}

string GetResDir() {
    return replace_filename(GMFN(), "..\\file.ext");
}

bool WinBrowseResource(const char* ename) {
	string target(ename);
	if (target.find("http:", 0) == 0 ||
		target.find("https:", 0) == 0 )
	{
	}
	else if (target.find("file://", 0) == 0) {
		target = target.substr(7);
	}
	else {
	  target = replace_filename(GetResDir(), ename);
	}
	ShellExecute(NULL, "open", target.c_str(), NULL, NULL, SW_SHOW);
	return true;
}

bool WinReadResText(const char * ename, std::string& result) {

	auto path = replace_filename(GMFN(), ename);
	FILE* f = fopen(path.c_str(), "r");
	if (!f) {
		string error{ "Can't open text resource: " + path };
		MessageBox(NULL, error.c_str(), "NXSYS", MB_ICONWARNING);
		return false;
	}
	std::string temp;
	std::vector<char>b(1000);
	while (true) {
		size_t haveRead = fread(&b[0], 1, b.size(), f);
		temp.append(&b[0], haveRead);
		if (haveRead < b.size())
			break;
	}
	fclose(f);
	result.clear();
	for (std::string::const_iterator i = temp.begin(); i != temp.end(); i++) {
		char c = *i;
		if (c == '\r') {
		}
		else if (c == '\n') {
			result += "\r\n";
		}
		else {
			result += c;
		}
	}
	return true;
}	
