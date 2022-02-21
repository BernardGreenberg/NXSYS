#ifndef  WIN32
#error RecentFileMan.h appearing in non-Windows build!
#endif

#pragma once
#include <string>
#include <Windows.h>

/* BSG 2/19/2022 */

struct RecentFileResult { bool valid; std::string pathname; };
void InitMenuRecentFiles(HMENU submenu);
RecentFileResult HandleRecentFileClick(UINT command);
void AssertRecentFileUse(const std::string& pathname);

