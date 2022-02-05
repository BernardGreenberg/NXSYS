#include <vector>
#include <string>
#include <stdlib.h>
#include <windows.h>
#include <incexppt.h>
#include <WinReadResText.h>


bool WinReadResText(const char * path, std::string& result) {
	std::vector<char> mpath(1024);
	std::string expanded_path;
	GetModuleFileName(NULL, &mpath[0], mpath.size());
	include_expand_path(&mpath[0], path, expanded_path);
	FILE* f = fopen(expanded_path.c_str(), "r");
	if (!f) {
		std::string error("Can't open text resource: ");
		error += expanded_path;
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