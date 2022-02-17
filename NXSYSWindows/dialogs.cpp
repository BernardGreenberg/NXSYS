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
#include "resource.h"
#include "ssdlg.h"
#include "dialogs.h"
#include "nxsysapp.h"
#include <string>
#include <vector>
#include <winver.h>
#include <regex>
#include <cassert>
#include "getmodtm.h"
#include "StatusReport.h"
#include "Resources2022.h"
#include "nxgo.h"
#include "WinApiSTL.h"
#include "STLExtensions.h"
#include "MessageBox.h"

#define TIME_FORMAT_STR "%#d %b %Y %H:%M"
//#include <getmodtm.h>
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

using std::string;
extern struct tm far nxtime_time_;
const int MaxFilesMenu = 20;

static std::regex relay_regex(R"(\s*(\d+\w+)\s*)");

static const char DemoFilter[] = "Interlocking Demos (*.xdo)\0*.xdo\0All Files (*.*)\0*.*\0\0";
static const char ScriptFilter[] = "NXScript scripts (*.nxs)\0*.nxs\0All Files (*.*)\0*.*\0\0";
static const char szFilter[] = "Track Layouts (*.tko)\0*.TKO\0Expr Code (*.TRK)\0*.TRK\0Both (.TRK/.TKO)\0*.TRK;*.TKO\0All Files (*.*)\0*.*\0\0";
static const char* InterpFilter = "Interpreted Xlkgs (*.trk)\0*.TRK\0All Files (*.*)\0*.*\0\0";
static const char* BatFilter = "Batch files (*.BAT)\0*.BAT\0All Files (*.*)\0*.*\0\0";

struct FDlgReturnVal FileOpenDlgSTL(HWND hWnd, const std::string file_name, const std::string title, bool rsw, FDlgExt ext) {
	char tbuff[MAX_PATH]{};
	char pbuff[MAX_PATH]{};
	strncpy(tbuff, title.c_str(), sizeof(tbuff) - 1);
	strncpy(pbuff, file_name.c_str(), sizeof(pbuff) - 1);

	if (FileOpenDlg(hWnd, pbuff, tbuff, sizeof(pbuff), rsw ? 1 : 0, ext))
		return FDlgReturnVal(pbuff, tbuff);
	else return FDlgReturnVal();
}

BOOL FileOpenDlg(HWND hwnd, LPSTR lpstrFileName, LPSTR lpstrTitleName, int bufl, int rsw, FDlgExt dsw) {

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
	case FDlgExt::Interpreted:
		ofn.lpstrFilter = InterpFilter;
		break;
	case FDlgExt::XDO:
		ofn.lpstrFilter = DemoFilter;
		break;
	case FDlgExt::NXScript:
		ofn.lpstrFilter = ScriptFilter;
		break;
	case FDlgExt::ShellScript:
		ofn.lpstrFilter = BatFilter;
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
	case FDlgExt::Layout:
	    ofn.lpstrTitle = "Open Interlocking definition file.";
	    break;
	case FDlgExt::Interpreted:
	    ofn.lpstrTitle = "Open NXSYS-Lisp code for interlocking definition";
	    break;
	case FDlgExt::XDO:
	    ofn.lpstrTitle = "Run Interlocking Demo file.";
	    break;
	case FDlgExt::ShellScript:
		ofn.lpstrTitle = "Shell Script (.BAT file)";
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
		return FALSE;

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
		GetFileVersionInfo(Path, NULL, (DWORD)version_data_len, vdp);
		VS_FIXEDFILEINFO    *pFileInfo = NULL;
		UINT                puLenFileInfo = 0;
		VerQueryValue(vdp, TEXT("\\"),(LPVOID*)&pFileInfo, &puLenFileInfo);
		WORD fv1 = HIWORD(pFileInfo->dwFileVersionMS);
		WORD fv2 = LOWORD(pFileInfo->dwFileVersionMS);
		WORD fv3 = HIWORD(pFileInfo->dwFileVersionLS);
		WORD fv4 = LOWORD(pFileInfo->dwFileVersionLS);
		char sversion[128];
		sprintf_s<COUNTOF(sversion)>
			(sversion, "Version %d.%d.%d.%d (MS Windows 10, %d bit)", fv1, fv2, fv3, fv4,
#ifdef _WIN64
			64);
#else
			32);
#endif
		SetDlgItemText(dialog, ABOUT_NUM_VSN, sversion);
		const char * build_type = 
#ifdef _DEBUG
			"Debug"
#else
			"Release"
#endif
			;

		sprintf_s <COUNTOF(sversion)>(sversion, "Built (%s)", build_type);
		SetDlgItemText(dialog, ABOUT_BUILD, sversion);
		UINT cbLang{};
		WORD* langInfo = nullptr;;

		VerQueryValue(vdp, "\\VarFileInfo\\Translation", (LPVOID*)&langInfo, &cbLang);
		//Prepare the label -- default lang is bytes 0 & 1 of langInfo
		sprintf_s<COUNTOF(sversion)>(sversion, "\\StringFileInfo\\%04x%04x\\%s",
			langInfo[0], langInfo[1], "ProductName");
		//Get the string from the resource data
		LPTSTR lpProdName = nullptr;
		UINT cbBufSize{};
		if (VerQueryValue(vdp, sversion, (LPVOID*)&lpProdName, &cbBufSize))	
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

INT_PTR CALLBACK CTDlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
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
};

static UINT ShowStopsLast;

INT_PTR CALLBACK show_stops_DlgProc(HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
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
	ChooseTrackDlg = CreateDialog(app_instance, "ChooseTrack", G_mainwindow, CTDlgProc);
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

