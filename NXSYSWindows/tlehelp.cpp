#include <windows.h>
#include "tlecmds.h"
#include "compat32.h"
#include <getmodtm.h>
#include <tledit.h>
#include <nxproduct.h>
#include "resource.h"
#include <string>
#include "WinReadResText.h"
#include "TlDlgProc.h"

COLORREF HelpColor = RGB(0, 228, 228);
HBRUSH HelpBrush = NULL;

static const char* BuildType =
#ifdef _DEBUG
"Debug"
#else
"Release"
#endif
#ifdef _WIN64
" (64-bit)"
#else
" (32-bit)"
#endif
" build";

static WNDPROC wpOrigEditProc;
static WNDPROC wpOrigDlgWndProc;
#define ALPHAS \
	       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_"

#define DATE_FORMAT "%#m/%#d/%Y %H:%M"

static void  ExpandDocString(const char * s, std::string& B);

void ResourceScat(UINT id, std::string& B) {
	char buf[1000];
	LoadString(GetModuleHandle(NULL), id, buf, sizeof(buf));
	ExpandDocString(buf, B);	/* better not recurse infinitely. */
}

static void
InterpretDocCmd(const char * p, int len, std::string& B) {
	char b[50];
	char cmd[33];
	if (len == 0 || len > sizeof(cmd) - 1) {
	fail:
		B += '%';
		if (len > 0)
			B.append(p, 0, len);
		return;
	}
#if 0
	else if (!stricmp(cmd, "Version"))
		B.scat(RJ_Version);
	else if (!stricmp(cmd, "EncryptionError"))
		B.scat(EncryptionErrorMessage);

#endif
    if (!stricmp(cmd, "PNAME"))
		B += PRODUCT_NAME;
	else if (!stricmp(cmd, "ExpireTime")) {
		b;
	}
	else
		goto fail;
}


static void
ExpandDocString(const char * s, std::string& B) {
	while (1) {
		const char * p = strchr(s, '%');
		if (p == NULL)
			break;
		else if (p[1] == '%') {
			if (p > s)
				B.append(s, 0, p - s);
			s = p + 2;
			B += '%';
		}
		else {
			if (p > s)
				B.append(s, 0, p - s);
			p++;
			int len = (int)strspn(p, ALPHAS);
			InterpretDocCmd(p, len, B);
			s = p + len;
			if (*s == ';')
				s++;
		}
	}
	B += s;
	return;
}


/* crockamarola to prevent text in edit box from selecting */
static BOOL_DLG_PROC_QUAL
EditBoxSubclassProc
(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	LRESULT lv = CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
	if (uMsg == WM_GETDLGCODE)
		lv &= ~DLGC_HASSETSEL;
	return lv;
}

static BOOL_DLG_PROC_QUAL
DlgWndProcSubclassProc
(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_CTLCOLORSTATIC || uMsg == WM_CTLCOLOREDIT)
	{
		HDC hDC = (HDC)wParam;
		HWND hWnd = (HWND)lParam;
		SetBkColor(hDC, HelpColor);
		return (LRESULT)HelpBrush;
	}
	return CallWindowProc(wpOrigDlgWndProc, hwnd, uMsg, wParam, lParam);
}


static BOOL_DLG_PROC_QUAL
HelpDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam) {

	HWND edit;
	switch (message) {

	case WM_INITDIALOG:
	{
		HelpBrush = CreateSolidBrush(HelpColor);
		edit = GetDlgItem(dialog, IDC_TLEDIT_TEXT);
		wpOrigDlgWndProc
			= (WNDPROC)SetWindowLongPtr
			(dialog, GWLP_WNDPROC, (LONG_PTR)DlgWndProcSubclassProc);
		if (edit) {
			wpOrigEditProc
				= (WNDPROC)SetWindowLongPtr
				(edit, GWLP_WNDPROC, (LONG_PTR)EditBoxSubclassProc);
			std::string BRaw, B;
			if (WinReadResText("tlehlptx.txt", BRaw)) {
				ExpandDocString(BRaw.c_str(), B);
				SetWindowText(edit, B.c_str());
			}
			else {
				EndDialog(dialog, 0);
				return FALSE;
			}
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
		edit = GetDlgItem(dialog, IDC_TLEDIT_TEXT);
		SetWindowLongPtr(dialog, GWLP_WNDPROC, (LONG_PTR)wpOrigDlgWndProc);
		if (edit) {
			SetWindowLongPtr(edit, GWLP_WNDPROC, (LONG_PTR)wpOrigEditProc);
			return TRUE;
		}
		DeleteObject(HelpBrush);
		return FALSE;

	default:
		return FALSE;
	}
}


static BOOL_DLG_PROC_QUAL
AboutDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam) {

	switch (message) {
	case WM_INITDIALOG:
	{
		char atime[40];
		time_t modtime = GetModuleTime(app_instance);
		strftime(atime, sizeof(atime), DATE_FORMAT, localtime(&modtime));
		SetDlgItemText(dialog, ABOUT_VSN, atime);
		SetDlgItemText(dialog, IDC_BUILD_TYPE, BuildType);
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
		case CmQuit:
			EndDialog(dialog, 0);
			PostQuitMessage(0);
			return TRUE;
		default:
			return FALSE;
		}
		return FALSE;

	default:
		return FALSE;
	}
}

void DoHelpDialog() {
	DialogBox(app_instance, MAKEINTRESOURCE(IDD_TLEDIT_HELP), G_mainwindow, HelpDlgProc);
}

void DoAboutDialog() {
	DialogBox(app_instance, MAKEINTRESOURCE(IDD_ABOUT), G_mainwindow, AboutDlgProc);
}

