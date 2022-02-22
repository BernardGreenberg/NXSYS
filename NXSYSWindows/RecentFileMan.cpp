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

struct FileEntry {
	FileEntry(string path) : Path(path) {
		Display = Path.filename().string();
	}
	fs::path Path;
	string Display;
};
static vector<FileEntry> Files;

static HMENU HMenu = nullptr;

static string Regkey(int index) {
	return "RecentPath_" + std::to_string(index);
}

static void SetClearCommand() {
	DeleteMenu(HMenu, ID_CLEAR_RECENT_FILE_LIST, MF_BYCOMMAND);
	DeleteMenu(HMenu, ID_SEPARATOR_RECENT_FILE_LIST, MF_BYCOMMAND);
	if (Files.size() > 0) {
		AppendMenu(HMenu, MF_SEPARATOR, ID_SEPARATOR_RECENT_FILE_LIST, NULL);
		InsertMenu(HMenu, -1, MF_BYPOSITION, ID_CLEAR_RECENT_FILE_LIST, "&0 Clear this list");
	}
};
static void PutUpDisambiguatedMenu() {
	std::unordered_multiset<string>multie;
	for (auto& file : Files)
		multie.insert(file.Display);
	int i = 0;
	for (auto& file: Files) {
		string tentative = file.Display;
		if (multie.count(tentative) > 1)
			tentative = tentative + " (" + file.Path.string() + ")";
		tentative = "& " + std::to_string(i+1) + " " + tentative;
		InsertMenu(HMenu, -1, MF_BYPOSITION, ID_RECENT_BASE + i, tentative.c_str());
		i++;
	}
}

void InitMenuRecentFiles(HMENU submenu) { 
   /* this goofy hash table accounts for multiple versions of seemingly the same path in the
      registry, which happened because of getStringRegval not trimming trailing zeros (fixed) */
	std::unordered_set<string>sfile_paths;
	HMenu = submenu;
	DeleteMenu(submenu, ID_1DUMY, MF_BYCOMMAND);
	int count = 0;
	Files.clear();
	AppKey hk("Settings");
	auto ctr = getDWORDRegVal(hk, "RecentFileCount");
	if (ctr.valid)
	    count = NXMIN((int)ctr.value, MAX_COUNT);
	for (int i = FIRST_INDEX; i < count;i++) {
		auto srslt = getStringRegval(hk, Regkey(i));
		if (srslt.valid) {
			string lresult = stolower(srslt.value);
			if (sfile_paths.count(lresult) == 0) {
				sfile_paths.insert(lresult);
				Files.push_back(lresult);
			}
		};
	}
	PutUpDisambiguatedMenu();
	
	SetClearCommand();
}

static void RefreshMenu() {
	AppKey ak("Settings");
	for (int index = LAST_INDEX+2; index >= FIRST_INDEX; index--)
		DeleteMenu(HMenu, index, MF_BYPOSITION);
	for (int index = FIRST_INDEX; index < Files.size(); index++)
		putStringRegval(ak, Regkey(index), Files[index].Path.string());
	PutUpDisambiguatedMenu();
	SetClearCommand();
	PutDWORDRegval(ak, "RecentFileCount", (DWORD)Files.size());
}

void AssertRecentFileUse(const std::string& pathname) {
	string lpath = stolower(pathname);
	for (auto it = Files.begin(); it != Files.end(); it++) {
		if (it->Path.string() == lpath) {
			Files.erase(it);
			break;  /* "it" becomes invalid */
		}
	}
	Files.insert(Files.begin(), lpath);
	RefreshMenu();
}

RecentFileResult HandleRecentFileClick(UINT command) {
	if (command == ID_CLEAR_RECENT_FILE_LIST) {
		Files.clear();
		RefreshMenu();
		return RecentFileResult{ false, "" };
	}
	if (command >= ID_RECENT_BASE && command < ID_RECENT_BASE + Files.size())
		return RecentFileResult(true, Files[command - ID_RECENT_BASE].Path.string());
	return RecentFileResult{ false, "" };
}