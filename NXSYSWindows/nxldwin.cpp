#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>

#include "commands.h"
#include "resource.h"
#include "lisp.h"
#include "nxsysapp.h"
#include "compat32.h"
#include "ldraw.h"
#include "dialogs.h"
#include "relays.h"
#include "nxldwin.h"
#include "usermsg.h"
#include "rlymenu.h"
#include "LDRightClick.h"
#include "NXRegistry.h"

static HWND S_RelayGraphicsWindow = nullptr;

static const DWORD RelayGraphicsWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_VSCROLL |
    WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;
static const char* RelayGraphicsWindow_Class = "NXSYS:Relayg";
static const char* RelayGraphicsWindowName = "NXSYS Relay Draftsperson";

static void PlaceDrawing (HWND window) {
    if (!PlaceRelayDrawing()) {
	ClearRelayGraphics();
	if (!PlaceRelayDrawing()) {
	    MessageBox (0, "Requested relay circuit too big for "
			"current size of relay graphics window.  Make it longer and "
			"maybe NARROWER.", RelayGraphicsWindowName,
			MB_OK | MB_ICONSTOP);
	    return;
	}
	InvalidateRect (window, NULL, 1);
	return;
    }
    InvalidateRect (window, NULL, 0);
}

static void SetLocatorPath(HWND window) {
	auto result = FileOpenDlgSTL(window, "", "BAT file Source Locator", true, FDlgExt::ShellScript);
	if (result.valid) {
		AppKey key("Settings");
		if (key)
			putStringRegval(key, SOURCE_LOCATOR_SCRIPT_VALUE_NAME, result.Path);
		InitRelayGraphicsSourceClick();
	}
}


//Don't know why "static" doesn't work...
 WNDPROC_DCL RelayGraphicsWindow_WndProc
   (HWND window, unsigned message, WPARAM wParam, LPARAM lParam){

    RECT rc;
    switch (message) {

	case WM_LBUTTONDOWN:
	    if (RelayGraphicsMouse (wParam, LOWORD (lParam), HIWORD (lParam)))
		PlaceDrawing (window);
	    break;

	case WM_RBUTTONDOWN:
		if (int newcmd = RelayGraphicsRightClick(window, wParam, LOWORD(lParam), HIWORD(lParam)))
			RelayGraphicsWindow_WndProc(window, WM_COMMAND, MAKEWPARAM(newcmd, 0), 0);
		break;
	case WM_COMMAND:

	    switch (LOWORD(wParam)) {
		case CmTrClear:
		    ClearRelayGraphics();
		    InvalidateRect (window, NULL, 1);
		    break;
		case CmPauseDemo:
		    InvalidateRect (window, NULL, 0);
		    break;
		case CmTrHide:
		    ShowWindow (window, SW_HIDE);
		    break;
		case CmShowRelayCircuit:
		    AskForAndDrawRelay(window);
		    break;
		case CmSetLctrPath:
		    SetLocatorPath(window);
		    break;
		default:
		    break;
	    }
	    return 0;

	case WM_CHAR:
	    if ((wParam & 0x7F) == 'a')
		InvalidateRect (window, NULL, 1);
	    break;

	case WM_SIZE:

	    GetWindowRect (window, &rc);
	    DrawingSetPageSize (rc.right-rc.left, rc.bottom-rc.top,
				rc.right-rc.left, 0);
	    InvalidateRect (window, NULL, 1);
	    break;

	case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    HDC dc = BeginPaint (window, &ps);
	    SelectObject (dc, Fnt);
	    RenderRelayPage (dc);
	    EndPaint (window, &ps);
	    break;
	}
	case WM_CLOSE:
	    ShowWindow (window, SW_HIDE);
	    break;
	default:
	    return DefWindowProc (window, message, wParam, lParam);
    }
    return 0;

}

int CreateRelayGraphicsWindow () {
    RECT rc;
    GetWindowRect (GetDesktopWindow(), &rc);
    int dtw = rc.right-rc.left;
    int dth = rc.bottom-rc.top;

    int x = dtw / 16 + dtw / 64;
    int y = dth / 2;
    int width = (7 * dtw) / 8;
    int height = (2 * dth) / 3;
    S_RelayGraphicsWindow = CreateWindow
			    (RelayGraphicsWindow_Class, RelayGraphicsWindowName,
				RelayGraphicsWindowStyle,
				x, y, width, height,
			        G_mainwindow, NULL, app_instance, NULL);
    if (!S_RelayGraphicsWindow){
	MessageBox (0, "Can't create Relay Graphics Window.",
		    app_name, MB_OK | MB_ICONEXCLAMATION);
	return 0;
    }
    /* don't know why create doesn't get it ...*/
    GetWindowRect (S_RelayGraphicsWindow, &rc);
    DrawingSetPageSize (rc.right-rc.left, rc.bottom-rc.top,
			rc.right-rc.left, 0);
    InvalidateRect (S_RelayGraphicsWindow, NULL, 1);
    return 1;
}

extern bool haveDisassembly;

void RelayShowString(const char* rnm) {
    bool first_time = (S_RelayGraphicsWindow == NULL);
    if (!S_RelayGraphicsWindow && !CreateRelayGraphicsWindow())
	return;
    if (!DrawRelayFromName(rnm))
	return;
    if (!haveDisassembly)
	PlaceDrawing(S_RelayGraphicsWindow);
    ShowWindow(S_RelayGraphicsWindow, SW_SHOWNORMAL);
    InvalidateRect(S_RelayGraphicsWindow, NULL, FALSE);
}

void CheckRelayDisplay() {
    if (S_RelayGraphicsWindow != NULL) {
	InvalidateRect (S_RelayGraphicsWindow, NULL, 0);
    }
}

BOOL RegisterRelayLogicWindowClass (HINSTANCE hInstance) {
    WNDCLASS klass;
    memset (&klass, 0, sizeof(klass));

    klass.style          = CS_BYTEALIGNCLIENT | CS_CLASSDC;
    klass.lpfnWndProc    = (WNDPROC) RelayGraphicsWindow_WndProc;
    klass.cbClsExtra     = 0;
    klass.cbWndExtra     = 0;
    klass.hInstance      = hInstance;
    klass.hIcon          = LoadIcon (hInstance, "RELAYICON");
    klass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    klass.hbrBackground  = (HBRUSH)GetStockObject (WHITE_BRUSH);
    klass.lpszMenuName   = MAKEINTRESOURCE(IDR_RELAYMENU);
    klass.lpszClassName  = RelayGraphicsWindow_Class;

    return RegisterClass (&klass);
}



void AskForAndDrawRelay (HWND win) {
    auto p = RlyDialog(win, app_instance);
    if (p.first)
	RelayShowString (p.second.c_str());
}

void DraftsbeingCleanupForSystem() {
    CleanUpDrawing();
}

void DraftsbeingCleanupForOneLayout () {
    CleanUpDrawing();
    if (S_RelayGraphicsWindow)
	ShowWindow (S_RelayGraphicsWindow, SW_HIDE);
}



#if 0
int ShowStateRelaysForObject(int object_number, const char* classdesc) {
    Relay* r = ListRelaysForObjectDialog("Show State", classdesc, object_number);
    if (!r)
	return 0;
    ShowStateRelay(r);
    return 1;
}
int DrawRelaysForObject (int object_number, const char * classdesc) {
    Relay * r = ListRelaysForObjectDialog ("Draw", classdesc, object_number);
    if (!r)
	return 0;
    RelayShowString(r->RelaySym.PRep().c_str());
    return 1;
}
#endif
