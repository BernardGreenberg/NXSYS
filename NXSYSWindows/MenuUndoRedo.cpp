#include <windows.h>
#include <string>
#include "tledit.h"
#include "tlecmds.h"
#include "NXSYSWinApp.h"

static void HackMenu(UINT item, const char* text, const char* basic, const char* accel) {
	std::string S;
	HMENU toplev = GetMenu(AppWindow);
	HMENU hMenu = GetSubMenu(toplev, 1);
	UINT flags = MF_BYCOMMAND | MF_STRING;
	if (text == nullptr) {
		flags |= MF_DISABLED;
		S = std::string(basic) + "\t" + accel;
	}
	else
		S = std::string(text) + "\t" + accel;
	ModifyMenu(hMenu, item, flags, item, S.c_str());
}

void SetUndoRedoMenu(const char* undo_text, const char* redo_text) {
	HackMenu(CmUndo, undo_text, "Undo", "Ctrl+z");
	HackMenu(CmRedo, redo_text, "Redo", "Ctrl+y");
}
