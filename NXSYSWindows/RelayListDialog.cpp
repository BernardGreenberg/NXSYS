#include <windows.h>
#include "compat32.h"
#include "resource.h"
#include "dialogs.h"
#include "nxsysapp.h"
#include <string>
#include <vector>
#include "lisp.h"
#include "relays.h"
#include "usermsg.h"


#include "WinApiSTL.h"
#include "STLExtensions.h"
#include "MessageBox.h"

void RelayShowString(const char* rnm);
void ShowStateRelay(Relay*);
using std::string;

typedef std::vector<Relay*>RArray;

class RelayListDialog {
public:
	RelayListDialog(HWND par, HINSTANCE inst) :
		hInstance(inst), Parent(par) {};
	static INT_PTR staticDlgProc(HWND, UINT, WPARAM, LPARAM);
	Relay* run(string title, RArray& relays);

private:
	HINSTANCE hInstance;
	HWND Parent;
	RArray* Relays;
	string Title;
	HWND hDlg;
	Relay* result;
	INT_PTR DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void ExtractResult();
};

Relay* RelayListDialog::run(string title, RArray& relays) {
	Title = title;
	Relays = &relays;

	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_OBJECT_RELAY_LIST), Parent, 
		staticDlgProc, (INT_PTR)this);

	return result;
}

void RelayListDialog::ExtractResult() {
	int selx = SendDlgItemMessage(hDlg, IDC_RELAY_LIST, LB_GETCURSEL, 0, 0);
	if (selx == LB_ERR)
		MessageBox(hDlg, "No item selected", PRODUCT_NAME, MB_OK | MB_ICONEXCLAMATION);
	else
	{
		result = (Relay*)SendDlgItemMessage(hDlg, IDC_RELAY_LIST, LB_GETITEMDATA, selx, 0);
		EndDialog(hDlg, TRUE);
	}
}

INT_PTR RelayListDialog::DlgProc(HWND p_hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
	{
		hDlg = p_hDlg;
		SetWindowTextS(hDlg, Title);
		HWND lb = GetDlgItem(hDlg, IDC_RELAY_LIST);
		for (auto r : *Relays) {
			const char* rtype = redeemRlsymId(r->RelaySym.u.r->type);
			int ix = SendMessage(lb, LB_ADDSTRING, -1, (LPARAM)rtype);
			SendMessage(lb, LB_SETITEMDATA, ix, (LPARAM)r);
		}
		return TRUE;
	}
	case WM_COMMAND:
		if (wParam == IDOK) {
			ExtractResult();
			return TRUE;
		}
		else if (wParam == IDCANCEL) {
			result = NULL;
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		else if (NOTIFY_CODE(wParam, lParam) == LBN_DBLCLK) {
			ExtractResult();
			return TRUE;
		}
		else return FALSE;
	default:
		return FALSE;
	}
	return FALSE;
}

INT_PTR RelayListDialog::staticDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_INITDIALOG)
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
	return (((RelayListDialog*)GetWindowLongPtr(hWnd, GWLP_USERDATA))->DlgProc(hWnd, msg, wParam, lParam));
}

Relay* ListRelaysForObjectDialog
   (const char* funcdesc,
	const char* classdesc,
	int object_number) 
{
	RArray relays = get_relay_array_for_object_number(object_number);
	if (relays.size() == 0) {
		usermsg("No relays found for %s %d.", classdesc, object_number);
		return NULL;
	}
	RelayListDialog RLD (G_mainwindow, app_instance);

	return RLD.run(FormatString("%s relay for %s %d", funcdesc, classdesc, object_number), relays);
}


void ShowStateRelaysForObject(int object_number, const char* classdesc) {
	if (Relay* r = ListRelaysForObjectDialog("Show State", classdesc, object_number))
		ShowStateRelay(r);
}


void DrawRelaysForObject(int object_number, const char* classdesc) {
	if (Relay* r = ListRelaysForObjectDialog("Draw", classdesc, object_number))
		RelayShowString(r->RelaySym.PRep().c_str());
}