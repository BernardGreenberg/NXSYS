#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "compat32.h"
#include "commands.h"
#include "ssdlg.h"
#include "dialogs.h"
#include "nxsysapp.h"
#include <vector>
#include <winver.h>
#include <cassert>
#include "getmodtm.h"
#include "StatusReport.h"
#include "Resources2022.h"

#define TIME_FORMAT_STR "%#d %b %Y %H:%M"
//#include <getmodtm.h>
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

using std::string;
extern struct tm far nxtime_time_;
const int MaxFilesMenu = 20;

static const char DemoFilter[] = "Interlocking Demos (*.xdo)\0*.xdo\0All Files (*.*)\0*.*\0\0";
static const char ScriptFilter[] = "NXScript scripts (*.nxs)\0*.nxs\0All Files (*.*)\0*.*\0\0";
static const char szFilter[] = "Track Layouts (*.tko)\0*.TKO\0Expr Code (*.TRK)\0*.TRK\0Both (.TRK/.TKO)\0*.TRK;*.TKO\0All Files (*.*)\0*.*\0\0";
static const char *InterpFilter = "Interpreted Xlkgs (*.trk)\0*.TRK\0All Files (*.*)\0*.*\0\0";

#define RELAY_CACHE_STRING_MAX 32
#define RELAY_CACHE_LEN 40

static int RCfirst = 0;

typedef char tRelayCacheItem[RELAY_CACHE_STRING_MAX];

static tRelayCacheItem RelayCache[RELAY_CACHE_LEN];

static void SaveToRelayCache(HWND dlg, int item) {
	HWND ctl = GetDlgItem(dlg, item);
	for (int i = 0; i < RELAY_CACHE_LEN; i++) {
		if (SendMessage(ctl, CB_GETLBTEXT, (WPARAM)i, (LPARAM)RelayCache[i])
			== CB_ERR)
			RelayCache[i][0] = '\0';
	}
}


static void RestoreFromRelayCache(HWND dlg, int item) {
	if (!RCfirst++)
		RelayCache[0][0] = '\0';
	HWND ctl = GetDlgItem(dlg, item);
	for (int i = 0; i < RELAY_CACHE_LEN; i++) {
		if (RelayCache[i][0] == '\0')
			break;
		SendMessage(ctl, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)RelayCache[i]);
	}
}


BOOL FileOpenDlg(HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName,
	int bufl, int rsw, FDlgExt dsw) {

	OPENFILENAME ofn;
	char ext[MAXEXT];
	memset(&ofn, 0, sizeof(OPENFILENAME));
	fnsplit(lpstrFileName, NULL, NULL, NULL, ext);
	_strlwr(ext);

	ofn.nFilterIndex = 1;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = NULL;
	switch (dsw) {
	case FDE_Interpreted:
		ofn.lpstrFilter = InterpFilter;
		break;
	case FDE_XDO:
		ofn.lpstrFilter = DemoFilter;
		break;
	case FDE_NXScript:
		ofn.lpstrFilter = ScriptFilter;
		break;
	default:
		ofn.lpstrFilter = szFilter;
		if (!strcmp(ext, ".trk"))
			ofn.nFilterIndex = 2;
		break;
	}

	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.lpstrFile = lpstrFileName;
	ofn.nMaxFile = bufl;
	ofn.lpstrFileTitle = lpstrTitleName;
	ofn.nMaxFileTitle = 256;
	ofn.lpstrInitialDir = NULL;
	switch (dsw) {
	case FDE_Layout:
	    ofn.lpstrTitle = "Open Interlocking definition file.";
	    break;
	case FDE_Interpreted:
	    ofn.lpstrTitle = "Open NXSYS-Lisp code for interlocking definition";
	    break;
	case FDE_XDO:
	    ofn.lpstrTitle = "Run Interlocking Demo file.";
	    break;
	}
	assert(rsw);
	ofn.Flags = (rsw ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT)
		| OFN_HIDEREADONLY;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "";
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.hwndOwner = hwnd;

	return rsw ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
}
#include <string>
static std::string fixnl(std::string s) {
    std::string out;
    for (char c : s) {
	if (c == '\n')
	    out += "\r\n";
	else
	    out += c;
    }
    return out;
}

DLGPROC_DCL status_report_DlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
	SetDlgItemText(dialog, IDC_STATUS_REPORT, fixnl(InterlockingFileStatus().Report()).c_str());
	return TRUE;

    case WM_COMMAND:
	if (wParam == IDOK || wParam == IDCANCEL) {
	    EndDialog(dialog, TRUE);
	    return TRUE;
	}
    default:
	return FALSE;
    }
}

void StatusReportDialog (HWND win, HINSTANCE instance) {
    DialogBox(instance, MAKEINTRESOURCE(IDD_STATUS_REPORT), win, (DLGPROC)status_report_DlgProc);
}


DLGPROC_DCL about_box_DlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
	char atime[40];
	switch (message) {
	case WM_INITDIALOG:
	{
		time_t modtime = GetModuleTime(NULL);
		strftime(atime, COUNTOF(atime), TIME_FORMAT_STR, localtime(&modtime));
		SetDlgItemText(dialog, ABOUT_VSN, atime);
		char Path[_MAX_PATH]{};
		GetModuleFileName(NULL, Path, COUNTOF(Path));
	
		size_t version_data_len = GetFileVersionInfoSize(Path, NULL);
		std::vector<char> infov(version_data_len);

		LPVOID vdp = &infov[0];
		GetFileVersionInfo(Path, NULL, version_data_len, vdp);
		VS_FIXEDFILEINFO    *pFileInfo = NULL;
		UINT                puLenFileInfo = 0;
		VerQueryValue(vdp, TEXT("\\"),(LPVOID*)&pFileInfo, &puLenFileInfo);
		WORD fv1 = HIWORD(pFileInfo->dwFileVersionMS);
		WORD fv2 = LOWORD(pFileInfo->dwFileVersionMS);
		WORD fv3 = HIWORD(pFileInfo->dwFileVersionLS);
		WORD fv4 = LOWORD(pFileInfo->dwFileVersionLS);
		char sversion[128];
		sprintf_s<COUNTOF(sversion)>
			(sversion, "Version %d.%d.%d.%d (MS Windows 10, 32 bit)", fv1, fv2, fv3, fv4);
		SetDlgItemText(dialog, ABOUT_NUM_VSN, sversion);
		const char * build_type = (pFileInfo->dwFileFlags & VS_FF_DEBUG) ? "Debug" : "Release";
		sprintf_s <COUNTOF(sversion)>(sversion, "Built (%s)", build_type);
		SetDlgItemText(dialog, ABOUT_BUILD, sversion);
		UINT cbLang{};
		WORD* langInfo = nullptr;;
		return TRUE;
		VerQueryValue(vdp, "\\VarFileInfo\\Translation", (LPVOID*)&langInfo, &cbLang);
		//Prepare the label -- default lang is bytes 0 & 1
		//of langInfo
		sprintf_s<COUNTOF(sversion)>(sversion, "\\StringFileInfo\\%04x%04x\\%s",
			langInfo[0], langInfo[1], "ProductName");
		//Get the string from the resource data
		LPTSTR lpProdName = nullptr;
		UINT cbBufSize{};
		if (VerQueryValue(vdp, sversion, (LPVOID*)&lpProdName,&cbBufSize))	
		    SetDlgItemText(dialog, ABOUT_PRODUCT_NAME, lpProdName);
		return TRUE;
	}
	case WM_COMMAND:
		if (wParam == IDOK || wParam == IDCANCEL) {
			EndDialog(dialog, TRUE);
			return TRUE;
		}
		else return FALSE;

	default:
		return FALSE;
	}
}

DLGPROC_DCL NullDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (wParam == IDOK || wParam == IDCANCEL) {
			EndDialog(dialog, TRUE);
			return TRUE;
		}
		else return FALSE;
	default:
		return FALSE;
	}
}


void AboutDialog(HWND win, HINSTANCE instance) {
	DialogBox(instance, "about_box", win, (DLGPROC)about_box_DlgProc);
}


void NullDialog(HWND win, HINSTANCE instance, char* name) {
	DialogBox(instance, name, win, (DLGPROC)NullDlgProc);
}

DLGPROC_DCL CTDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (wParam == IDOK || wParam == IDCANCEL) {
			SendMessage(G_mainwindow, WM_NXSYS_SELTRACK, (WPARAM)-1, 0L);
			return TRUE;
		}
		else return FALSE;
	case WM_CHAR:
		wParam &= 0x7F;
		if (wParam >= '1' && wParam <= '9')
			SendMessage(G_mainwindow, WM_NXSYS_SELTRACK, wParam - '0', 0L);
		return TRUE;
	default:
		return FALSE;
	}
}

static char * S_rvptr;
static char LastRelayID[30] = "";

static int FinishRelayFld(HWND dlg, unsigned cmd) {
	int c = 0;
	char * pp;
	int textl = (WORD)SendDlgItemMessage
	(dlg, cmd, WM_GETTEXT, 30, (LPARAM)S_rvptr);
	S_rvptr[textl] = '\0';
	char *p = S_rvptr;
	while (isspace(*p)) p++;
	while (isdigit(*p)) {
		c = 1;
		p++;
	}
	if (c == 0) goto bad;
	c = 0;
	while (isalnum(*p)) {
		c = 1;
		*p = toupper(*p);
		p++;
	}
	if (c == 0) goto bad;
	pp = p;
	while (isspace(*p)) p++;
	if (*p != '\0') {
	bad:	char mbuf[100];
		wsprintf(mbuf, "Not a relay string: %s", S_rvptr);
		MessageBox(0, mbuf, "Dialog Field Value Error", MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}
	LPARAM lpString = (LPARAM)S_rvptr;
	SendDlgItemMessage(dlg, cmd, WM_SETTEXT, 0, lpString);
	int old_cbx = SendDlgItemMessage(dlg, cmd, CB_FINDSTRINGEXACT, -1, lpString);
	if (old_cbx != CB_ERR)
		SendDlgItemMessage(dlg, cmd, CB_DELETESTRING, (WPARAM)old_cbx, (LPARAM)0);
	SendDlgItemMessage(dlg, cmd, CB_INSERTSTRING, 0, lpString);
	*pp = '\0';
	strcpy(LastRelayID, S_rvptr);
	return 1;
}


DLGPROC_DCL relay_id_DlgProc(HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_RELAY_ID_EDIT, LastRelayID);
		RestoreFromRelayCache(hDlg, IDC_RELAY_ID_EDIT);
		return TRUE;
	case WM_COMMAND:
		if (wParam == IDCANCEL) {
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		else if (wParam == IDOK) {
			if (!FinishRelayFld(hDlg, IDC_RELAY_ID_EDIT))
				return TRUE;
			SaveToRelayCache(hDlg, IDC_RELAY_ID_EDIT);
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
		else if (NOTIFY_CODE(wParam, lParam) == CBN_SELENDOK) {
			char buf[32]{};
			int x = (int)SendDlgItemMessage(hDlg, IDC_RELAY_ID_EDIT, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hDlg, IDC_RELAY_ID_EDIT, CB_GETLBTEXT, x, (LPARAM)buf);
			SetDlgItemText(hDlg, IDC_RELAY_ID_EDIT, buf);
			return relay_id_DlgProc(hDlg, message, IDOK, 0);
		}
		else return FALSE;
	default:
		return FALSE;
	}
}

int RlyDialog(HWND win, HINSTANCE instance, char* buf) {
	S_rvptr = buf;
	return DialogBox(instance, "relay_id", win, (DLGPROC)relay_id_DlgProc);
}

static UINT ShowStopsLast;

DLGPROC_DCL show_stops_DlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
	UINT cmd;
	UINT i;
	switch (message) {
	case WM_INITDIALOG:
		for (i = SSDL_FIRST; i <= SSDL_LAST; i++)
			SendDlgItemMessage
			(dialog, i, BM_SETCHECK, (ShowStopsLast == i), 0L);
		return TRUE;

	case WM_COMMAND:
		cmd = LOWORD(wParam);
		if (cmd == IDOK) {
			EndDialog(dialog, TRUE);
			return TRUE;
		}
		else if (cmd == IDCANCEL) {
			EndDialog(dialog, FALSE);
			return TRUE;
		}
		else if (cmd >= SSDL_FIRST && cmd <= SSDL_LAST) {
			for (i = SSDL_FIRST; i <= SSDL_LAST; i++)
				SendDlgItemMessage
				(dialog, i, BM_SETCHECK, (cmd == i), 0L);
			ShowStopsLast = cmd;
			return TRUE;
		}
		else
			return FALSE;
	default:
		return FALSE;
	}
}



int ShowStopDlg(HWND win, HINSTANCE instance, int policy) {
	ShowStopsLast = (UINT)policy;
	if (DialogBox(instance, "ShowStops", win, (DLGPROC)show_stops_DlgProc))
		return (int)ShowStopsLast;
	else
		return 0;
}

HWND ChooseTrackDlg = NULL;


void OfferChooseTrackDlg() {
	ChooseTrackDlg = CreateDialog(app_instance, "ChooseTrack", G_mainwindow,
		CTDlgProc);
	if (ChooseTrackDlg != NULL)
		ShowWindow(ChooseTrackDlg, SW_SHOW);
	SetFocus(G_mainwindow);
}

void FlushChooseTrackDlg() {
	if (ChooseTrackDlg != NULL)
		DestroyWindow(ChooseTrackDlg);
	ChooseTrackDlg = NULL;
}


double VirtualScreenScale = 1.0;

#include <nxgo.h>

DLGPROC_DCL ScaleDlgProc(HWND hDlg, unsigned message, WPARAM wParam, LPARAM lParam)
{
	char buf[30];

	switch (message) {
	case WM_INITDIALOG:
	{
		if ((((int)(VirtualScreenScale*1000.0)) % 1000) == 0)
			sprintf(buf, "%d", (int)VirtualScreenScale);
		else
			sprintf(buf, "%g", VirtualScreenScale);
		SetDlgItemText(hDlg, IDC_SCALE_EDIT, buf);
		return TRUE;
	}
	case WM_COMMAND:
		switch (wParam) {
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		case IDOK:
			GetDlgItemText(hDlg, IDC_SCALE_EDIT, buf, sizeof(buf) - 1);
			if (sscanf(buf, "%lf", &VirtualScreenScale) == 1) {
			fx:			NXGO_SetDisplayScale(VirtualScreenScale);
				EndDialog(hDlg, TRUE);
				return TRUE;
			}
			else
				MessageBox(hDlg, "Bad number.  Try again.",
					"Display Scale Dialog",
					MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		case IDC_SCALE_P5:
			VirtualScreenScale = .5;
			goto fx;
		case IDC_SCALE_P7:
			VirtualScreenScale = .7;
			goto fx;
		case IDC_SCALE_P8:
			VirtualScreenScale = .8;
			goto fx;
		case IDC_SCALE_1:
			VirtualScreenScale = 1.0;
			goto fx;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}

BOOL ScaleDialog() {
	return DialogBox(app_instance, MAKEINTRESOURCE(IDD_SCALE),
		G_mainwindow, DLGPROC(ScaleDlgProc));
}

