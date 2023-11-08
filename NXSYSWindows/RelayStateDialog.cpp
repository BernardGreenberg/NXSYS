#include <windows.h>
#include "commands.h"
#include "resource.h"
#include "lisp.h"
#include "compat32.h"
#include "ldraw.h"
#include "dialogs.h"
#include "relays.h"
#include "nxldwin.h"
#include "rlymenu.h"
#include "nxsysapp.h"
#include "usermsg.h"
#include "WinApiSTL.h"
#include "STLExtensions.h"

/* Extracted out of dialogs.cpp and made a class, the way it should be 14 Feb 2022 */

class RelayStateDialog {
public:
	RelayStateDialog(HINSTANCE instance, HWND parent) : hParent(parent), app_instance(instance) {};
	void run(Relay* rly);
	static DLGPROC_DCL staticDlgProc(HWND hWnd, UINT message, WPARAM, LPARAM);
	bool valid;
private:
	HWND hParent = NULL;
	HINSTANCE app_instance = NULL;
	BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
	HWND hDlg = NULL;
	HWND hLB = NULL;
	Relay* relay = NULL;
	Relay* GetSelRelayFromListDlg();
	void DoRelay();

};

Relay* RelayStateDialog::GetSelRelayFromListDlg() {
	int selx = (int)SendMessage(hLB, LB_GETCURSEL, 0, 0);
	if (selx < 0)
		return nullptr;
	return (Relay*)SendMessage(hLB,LB_GETITEMDATA, selx, 0);
}

void RelayStateDialog::DoRelay() {
	EnableWindow(GetDlgItem(hDlg, IDC_DRAW_RELAY), !(relay->Flags & LF_CCExp));;
	SetDlgItemTextS(hDlg, IDC_RLYQUERY_NAME, "Relay " + relay->RelaySym.PRep());
	SetDlgItemTextS(hDlg, IDC_RLYQUERY_STATE, std::string{"State is "} + (relay->State ? "PICKED" : "DROPPED"));
	std::string depmsg = FormatString("%d dependent%s:",
		relay->Dependents.size(), (relay->Dependents.size()) == 1 ? "" : "s");
	SetDlgItemTextS(hDlg, IDC_RLYQUERY_NDEPS, depmsg);
	SendMessage(hLB, LB_RESETCONTENT, 0, 0);
	std::vector<Relay*>deps_copy = relay->Dependents;
	pointer_sort(deps_copy.begin(), deps_copy.end());
	for (const Relay* dep : deps_copy) {
		std::string d2msg = FormatString("%s\t%d", dep->RelaySym.PRep().c_str(), dep->State);
		int index = (int)SendMessage(hLB, LB_ADDSTRING, 0, (LPARAM)d2msg.c_str());
		SendMessage (hLB, LB_SETITEMDATA, index, (LPARAM)dep);
	}
}

BOOL RelayStateDialog::DlgProc(HWND p_hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		hDlg = p_hDlg;
		hLB = GetDlgItem(hDlg, IDC_RLYQUERY_LIST);
		DoRelay();
		return TRUE;

	case WM_COMMAND:
		if (wParam == IDOK || wParam == IDCANCEL) {
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		else if (wParam == IDC_DRAW_RELAY) {
			RelayShowString(relay->RelaySym.PRep().c_str());
			return TRUE;
		}
		else if (NOTIFY_CODE(wParam, lParam) == LBN_DBLCLK) {
			relay = GetSelRelayFromListDlg();
			DoRelay();
			return TRUE;
		}
		else return FALSE;
	default:
		return FALSE;
	}
}

DLGPROC_DCL RelayStateDialog::staticDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_INITDIALOG)
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
	return (((RelayStateDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA))->DlgProc(hDlg, message, wParam, lParam));
}

void RelayStateDialog::run(Relay* rly) {
	relay = rly;
	DialogBoxParam(app_instance, MAKEINTRESOURCE(IDD_RELAYSTATE),
		hParent, DLGPROC(staticDlgProc), (LPARAM)this);
}
 void ShowStateRelay(Relay* rly) {
	 RelayStateDialog(app_instance, G_mainwindow).run(rly);
}


void AskForAndShowStateRelay(HWND win) {
	auto p = RlyDialog(win, app_instance);
	if (p.first) {
		Sexpr s = RlysymFromStringNocreate(p.second.c_str());
		if (s == NIL || s.u.r->rly == NULL) {
			usermsg("No such relay: %s", p.second.c_str());
			return;
		}
		ShowStateRelay(s.u.r->rly);
	}
}
