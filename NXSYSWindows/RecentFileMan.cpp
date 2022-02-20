#include <Windows.h>
#include <string.h>
#include <vector>
#include "RecentFileMan.h"
#include "NXRegistry.h"
#include "resource.h"
#include "NXSYSMinMax.h"
#include "STLExtensions.h"
#include <filesystem>
#include <unordered_set>

using std::vector, std::string;
namespace fs = std::filesystem;

#define FIRST_INDEX 0
#define LAST_INDEX 8 
#define MAX_COUNT LAST_INDEX + 1

static vector<string> FilePaths;

static HMENU HMenu = nullptr;

static string LabelN(int index, const string& path) {
	fs::path fspath(path);
	return string{ "&" } + std::to_string(index + 1) + " " + fspath.filename().string();
}

static string Regkey(int index) {
	return "RecentPath_" + std::to_string(index);
}

static void SetClearCommand() {
	DeleteMenu(HMenu, ID_CLEAR_RECENT_FILE_LIST, MF_BYCOMMAND);
	DeleteMenu(HMenu, ID_SEPARATOR_RECENT_FILE_LIST, MF_BYCOMMAND);
	if (FilePaths.size() > 0) {
		AppendMenu(HMenu, MF_SEPARATOR, ID_SEPARATOR_RECENT_FILE_LIST, NULL);
		InsertMenu(HMenu, -1, MF_BYPOSITION, ID_CLEAR_RECENT_FILE_LIST, "&0 Clear this list");
	}
};

void InitMenuRecentFiles(HMENU submenu) { 
   /* this goofy hash table accounts for multiple versions of seemingly the same path in the
      registry, which happened because of getStringRegval not trimming trailing zeros (fixed) */
	std::unordered_set<string>sfile_paths;
	HMenu = submenu;
	DeleteMenu(submenu, ID_1DUMY, MF_BYCOMMAND);
	int count = 0;
	FilePaths.clear();
	AppKey hk("Settings");
	auto ctr = getDWORDRegVal(hk, "RecentFileCount");
	if (ctr.valid)
	    count = NXMIN((int)ctr.value, MAX_COUNT);
	for (int i = FIRST_INDEX; i < count;i++) {
		auto srslt = getStringRegval(hk, Regkey(i));
		if (srslt.valid) {
			int index = (int)FilePaths.size();
			string lresult = stolower(srslt.value);
			if (sfile_paths.count(lresult) == 0) {
				sfile_paths.insert(lresult);
				InsertMenu(submenu, -1, MF_BYPOSITION, ID_RECENT_BASE + index, LabelN(index, lresult).c_str());
				FilePaths.push_back(lresult);
			}
		};
	}
	SetClearCommand();
}

static void RefreshMenu() {
	AppKey ak("Settings");
	for (int index = LAST_INDEX+2; index >= FIRST_INDEX; index--)
		DeleteMenu(HMenu, index, MF_BYPOSITION);
	for (int index = FIRST_INDEX; index <= FilePaths.size(); index++) {
		int cmd = ID_RECENT_BASE + index;	
		if (index < (int)FilePaths.size()) {
			InsertMenu(HMenu, -1, MF_BYPOSITION, cmd, LabelN(index, FilePaths[index]).c_str());
			putStringRegval(ak, Regkey(index), FilePaths[index]);
		}
	}
	SetClearCommand();
	PutDWORDRegval(ak, "RecentFileCount", (DWORD)FilePaths.size());
}

void AssertRecentFileUse(const std::string& pathname) {
	string lpath = stolower(pathname);
	for (auto it = FilePaths.begin(); it != FilePaths.end(); it++) {
		if (*it == lpath)
			FilePaths.erase(it);
		break;  /* it becomes invalid */
	}
	FilePaths.insert(FilePaths.begin(), lpath);
	RefreshMenu();
}

RecentFileResult HandleRecentFileClick(UINT command) {
	if (command == ID_CLEAR_RECENT_FILE_LIST) {
		FilePaths.clear();
		RefreshMenu();
		return RecentFileResult{ false, "" };
	}
	if (command >= ID_RECENT_BASE && command < ID_RECENT_BASE + FilePaths.size())
		return RecentFileResult(true, FilePaths[command - ID_RECENT_BASE]);
	return RecentFileResult{ false, "" };
}