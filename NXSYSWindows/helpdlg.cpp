#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cassert>

#include "nxsysapp.h"
#include "helpdlg.h"
#include "usermsg.h"
#include "resource.h"
#include "WinReadResText.h"

/* 11 January 2001 */
/* thorough rewrite 19 Feb 2022 */

#define HELP_SUBMENU_X 5
using std::string, std::vector;

static WNDPROC wpOrigEditProc;
extern std::string HelpPath;

#define MAX_HELP_TEXTS 10

struct HelpText {
	HelpText(const char* text, const char* title) : Text(text), Title(title) {}
	HelpText(const char* text, const char* title, UINT hfid);
    string Title;
    string Text;
    int CmdId;
    void Display();
    void Remove(HMENU hHelpMenu);
    void Dialog();
};

static vector<struct HelpText> HelpTexts;

/* crockamarola to prevent text in edit box from selecting */
static UINT_PTR APIENTRY
   EditBoxSubclassProc
   (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    LRESULT lv = CallWindowProc (wpOrigEditProc, hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_GETDLGCODE)
		lv &= ~DLGC_HASSETSEL;
    return lv;
}


static INT_PTR CALLBACK
HelpDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam) {

	HWND edit;
	switch (message) {
	case WM_INITDIALOG:
	{
		HelpText* pHT = (HelpText*)lParam;
		SetWindowText(dialog, pHT->Title.c_str());
		edit = GetDlgItem(dialog, IDC_HELPDLG_TEXT);

		if (edit) {
			SetWindowText(edit, pHT->Text.c_str());
			wpOrigEditProc
				= (WNDPROC)SetWindowLongPtr
				(edit, GWLP_WNDPROC, (INT_PTR)EditBoxSubclassProc);
		}
		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(dialog, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(dialog, 0);
			return TRUE;
		default:
			return FALSE;
		}
		return FALSE;

	case WM_DESTROY:
		edit = GetDlgItem(dialog, IDC_HELPDLG_TEXT);
		if (edit) {
			SetWindowLongPtr(edit, GWLP_WNDPROC, (INT_PTR)wpOrigEditProc);
			return TRUE;
		}
		return FALSE;

	default:
		return FALSE;
	}
}

void HelpText::Dialog () {
    DialogBoxParam (app_instance, MAKEINTRESOURCE(IDD_HELPDLG),
		    G_mainwindow, HelpDlgProc, (LPARAM) this);
}    

void HelpDialog (const char * text, const char * title) {
	HelpText( text, title ).Dialog();
}

void HelpText::Display() {
	if (Text.length() >= 5 && Text.substr(0, 5) == "file:")
		WinBrowseResource(Text.c_str());
	else
		Dialog();
}

void HelpText::Remove (HMENU hHelpMenu) {
    DeleteMenu (hHelpMenu, CmdId, MF_BYCOMMAND);
}


void DisplayHelpTextByCommand (UINT Cmd) {
    HelpTexts[Cmd - ID_EXTHELP0].Display();
}

static HMENU GetHelpSubMenu () {
    HMENU hMenu = GetMenu (G_mainwindow);
    return GetSubMenu (hMenu, HELP_SUBMENU_X);
}

HelpText::HelpText (const char * text, const char * title, UINT hfid) {
	if (text) Text = text;
	if (title) Title = title;
	(void)hfid; /* WinHelp no longer exists. */
	CmdId = UINT(int(ID_EXTHELP0) + (int)HelpTexts.size());
    HMENU hHelpMenu = GetHelpSubMenu();
    if (CmdId == ID_EXTHELP0) //Good idea, Henry
		AppendMenu (hHelpMenu, MF_SEPARATOR, NULL, 0);
    AppendMenu (hHelpMenu, MF_STRING | MF_ENABLED, CmdId, Title.c_str());
}    

void RegisterHelpMenuText (const char * text, const char * title) {
	HelpTexts.emplace_back (text, title, 0);
}

void RegisterInHelpfileMenuItem (const char * title, UINT helpfile_id) {
	HelpTexts.emplace_back ("", title, helpfile_id);
}

void ClearHelpMenu () {
    HMENU hHelpMenu = GetHelpSubMenu();
	for (auto& help : HelpTexts)	
		help.Remove(hHelpMenu);
    DeleteMenu (hHelpMenu, 0, MF_SEPARATOR);
}

