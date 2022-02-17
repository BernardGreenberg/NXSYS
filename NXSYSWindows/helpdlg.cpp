#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "nxsysapp.h"
#include "helpdlg.h"
#include "usermsg.h"
#include "resource.h"

/* 11 January 2001 */

#ifdef NXV2
#define HELP_SUBMENU_X 5
#else
#define HELP_SUBMENU_X 6
#endif

static WNDPROC wpOrigEditProc;
extern std::string HelpPath;

#define MAX_HELP_TEXTS 10

struct HelpText {
    const char * Title;
    const char * Text;
    UINT FixedId;
    UINT CmdId;
    void Setup (const char * text, const char * title, UINT hfid);
    void Display();
    void Remove(HMENU hHelpMenu);
    void Dialog();
};

static HelpText HelpTexts[MAX_HELP_TEXTS];
static int NHelpTexts = 0;


/* crockamarola to prevent text in edit box from selecting */
static BOOL APIENTRY
   EditBoxSubclassProc
   (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    LRESULT lv = CallWindowProc (wpOrigEditProc, hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_GETDLGCODE)
	lv &= ~DLGC_HASSETSEL;
    return lv;
}


static INT_PTR CALLBACK
   HelpDlgProc (HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam) {

    HWND edit;
    switch (message) {
	case WM_INITDIALOG:
	{
	    HelpText * pHT = (HelpText*)lParam;
	    SetWindowText (dialog, pHT->Title);
	    edit = GetDlgItem(dialog, IDC_HELPDLG_TEXT);

	    if (edit) {
		SetWindowText (edit, pHT->Text);
		wpOrigEditProc
			= (WNDPROC) SetWindowLongPtr
			  (edit, GWLP_WNDPROC, (INT_PTR) EditBoxSubclassProc);
	    }
	    return TRUE;
	}
	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case IDOK:
		    EndDialog (dialog, 1);
		    return TRUE;
		case IDCANCEL:
		    EndDialog (dialog, 0);
		    return TRUE;
		default:
		    return FALSE;
	    }
	    return FALSE;

	case WM_DESTROY:
	    edit = GetDlgItem(dialog, IDC_HELPDLG_TEXT);
	    if (edit) {
		SetWindowLongPtr(edit, GWLP_WNDPROC, (INT_PTR) wpOrigEditProc);
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
    HelpText HT;
    HT.Title = title;
    HT.Text = text;
    HT.Dialog();
}

void HelpText::Display () {
    if (FixedId)
	WinHelp (G_mainwindow, HelpPath.c_str(), HELP_CONTEXT, FixedId);
    else
	Dialog();
}

void HelpText::Remove (HMENU hHelpMenu) {
    if (Title)
	free((PVOID)Title);
    if (Text)
	free((PVOID)Text);
    DeleteMenu (hHelpMenu, CmdId, MF_BYCOMMAND);
}


void DisplayHelpTextByCommand (UINT Cmd) {
    HelpTexts[Cmd - ID_EXTHELP0].Display();
}

static HMENU GetHelpSubMenu () {
    HMENU hMenu = GetMenu (G_mainwindow);
    return GetSubMenu (hMenu, HELP_SUBMENU_X);
}

static bool CheckNotTooManyHelps (LPCSTR title) {
    if (NHelpTexts >= MAX_HELP_TEXTS) {
	usermsg ("Maximum number of help menu texts (%d) exceeded. Can't"
		 " put text \"%s\" in help menu.",
		 MAX_HELP_TEXTS, title);
	return false;
    }
    return true;
}

void HelpText::Setup (const char * text, const char * title, UINT hfid) {
    Text = text ? _strdup(text) : NULL;
    Title = _strdup(title);
    FixedId = hfid;
    CmdId = ID_EXTHELP0 + this - HelpTexts;
    HMENU hHelpMenu = GetHelpSubMenu();
    if (CmdId == ID_EXTHELP0) //Good idea, Henry
	AppendMenu (hHelpMenu, MF_SEPARATOR, NULL, 0);
    AppendMenu (hHelpMenu, MF_STRING | MF_ENABLED, CmdId, Title);
}    

void RegisterHelpMenuText (const char * text, const char * title) {
    if (CheckNotTooManyHelps(title))
	HelpTexts[NHelpTexts++].Setup (text, title, 0);
}

void RegisterInHelpfileMenuItem (const char * title, UINT helpfile_id) {
    if (CheckNotTooManyHelps(title))
	HelpTexts[NHelpTexts++].Setup (NULL, title, helpfile_id);
}

void ClearHelpMenu () {
    HMENU hHelpMenu = GetHelpSubMenu();
    for (int i = 0; i < NHelpTexts; i++)
	HelpTexts[i].Remove(hHelpMenu);
    DeleteMenu (hHelpMenu, 0, MF_SEPARATOR);
    NHelpTexts = 0;
}

