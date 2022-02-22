#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include "compat32.h"
#include "commands.h"
#include "resource.h"
#include "dialogs.h"
#include <string>
#include <vector>
#include <regex>
#include <cassert>
#include "WinApiSTL.h"
#include "STLExtensions.h"
#include "MessageBox.h"

using std::string;
typedef std::pair<bool, string> RQDResult;
static std::regex relay_regex(R"(\s*(\d+\w+)\s*)");

class RelayQueryDialog {
	HWND hDlg;
	HWND hLB;
	HWND hEdit;
	BOOL DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool AcceptValue();
	void SaveToCache();
	static std::vector<string> RelayCache;
public:
	RelayQueryDialog(HINSTANCE hMod, HWND mainWnd);
	static DLGPROC_DCL staticDlgProc(HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam);
	bool valid;
	string return_value;

};

static string LastRelayID;
std::vector<string>RelayQueryDialog::RelayCache;

bool RelayQueryDialog::AcceptValue() {
	string S = stoupper(GetDlgItemText(hDlg, IDC_RELAY_ID_EDIT));
	std::smatch match;
	if (!std::regex_match(S, match, relay_regex)) {
		MessageBoxS(0, "Not a relay string: " + S, "Dialog Field Value Error", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return_value = match.str(1);
	const char* cs = return_value.c_str();
	SetWindowTextS(hLB, return_value);
	int old_cbx = (int)SendMessage(hLB, CB_FINDSTRINGEXACT, -1, (LPARAM)cs);
	if (old_cbx != CB_ERR)
		SendMessage(hLB, CB_DELETESTRING, (WPARAM)old_cbx, 0);
	SendMessage(hLB, CB_INSERTSTRING, 0, (LPARAM)cs);
	SaveToCache();
	return true;
}



BOOL RelayQueryDialog::DlgProc(HWND p_hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_INITDIALOG:
		hDlg = p_hDlg;
		hEdit = GetDlgItem(hDlg, IDC_RELAY_ID_EDIT);
		hLB = hEdit;
		for (size_t i = 0; i < RelayCache.size(); i++)
			SendMessage(hLB, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)RelayCache[i].c_str());
		return TRUE;
	case WM_COMMAND:
		if (wParam == IDCANCEL) {
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		else if (wParam == IDOK) {
			if (AcceptValue())
				EndDialog(hDlg, TRUE);
			return TRUE;
		}
		else if (NOTIFY_CODE(wParam, lParam) == CBN_SELENDOK) {
			char buf[32]{};
			int cursel = (int)SendDlgItemMessage(hDlg, IDC_RELAY_ID_EDIT, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hDlg, IDC_RELAY_ID_EDIT, CB_GETLBTEXT, cursel, (LPARAM)buf);
			SetDlgItemText(hDlg, IDC_RELAY_ID_EDIT, buf);
			return DlgProc(hDlg, message, IDOK, 0);
		}
		else return FALSE;
	default:
		return FALSE;
	}
}

DLGPROC_DCL RelayQueryDialog::staticDlgProc(HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_INITDIALOG)
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
	RelayQueryDialog* pRQD = (RelayQueryDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return pRQD->DlgProc(hDlg, message, wParam, lParam);
}

RQDResult  RlyDialog(HWND win, HINSTANCE instance) {
	RelayQueryDialog RQD (instance, win);
	if (RQD.valid)
		return RQDResult(true, RQD.return_value);
	else
		return RQDResult(false, "Dialog Creation Failed.");
}
RelayQueryDialog::RelayQueryDialog(HINSTANCE instance, HWND win) {
	valid = (0 < DialogBoxParam(instance, MAKEINTRESOURCE(IDD_RELAY_ID), win,
		         (DLGPROC)staticDlgProc, (LPARAM)this));
}

void RelayQueryDialog::SaveToCache() {
	RelayCache.clear();
	for (int i = 0; ; i++) {
		int res = (int)SendMessage(hLB, CB_GETLBTEXTLEN, (WPARAM)i, 0);
		if (res == CB_ERR)
			break;
		std::vector<char>buf(res + 1);
		if (CB_ERR == SendMessage(hLB, CB_GETLBTEXT, (WPARAM)i, (LPARAM)buf.data()))
			break;
		else
			RelayCache.push_back(buf.data());
	}
}