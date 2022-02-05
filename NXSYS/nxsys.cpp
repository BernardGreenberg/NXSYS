#define NXSYS_APP_BASE_MINOR_VERSION 3

#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "nxsysapp.h"
#include "commands.h"
#include "nxgo.h"
#include "timers.h"
#include "objid.h"
#include "compat32.h"
#include "lyglobal.h"
#include "loaddcls.h"
#include "nxglapi.h"
#include "demoapi.h"
#include "dialogs.h"
#include "ssdlg.h"
#include "rlyapi.h"
#include "nxldapi.h"
#include "fsigctl.h"
#include "nxldwin.h"
#include "rlytrapi.h"
#include "usermsg.h"
#include "nxhelp.h"
#include "helpdlg.h"
#include "resource.h"
#include <string>
#include "incexppt.h"
#include "STLExtensions.h"
#include "STLfnsplit.h"
#include "AppAbortRestart.h"

#ifdef NXSYSMac
char ** ParseArgString(const char *);
int ParseArgsArgCount(char **);
void ParseArgsFree(char **);
#include "AppDelegateGlobals.h"
#else
#include "WinReadResText.h"
#endif

#ifdef NXOLE
#include "nxole.h"
#endif

#ifndef NOTRAINS
#include "trainapi.h"
#endif
#ifdef NXV2
#include "dynmenu.h"
BOOL EnableAutoOperation = FALSE;
#endif

#ifdef EVALUATION_EDITION
#include <tcryptxf.h>
#include <tcrptsym.h>
long RTExpireTime = 0; //0x35D57840;
#endif

#if WINDOWS
#include <parsargs.h>
#include <heapchk.h>
#endif

void ValidateRelayWorld();

/* time to wait for Normal All Switches */
#define NORMAL_ALL_WAIT_TIME_MS 1500L

#ifdef RT_PRODUCT
 #define INITIAL_BLURB "Please use   Help | Info  for usage information."
 #define NXSYS_LM_REG_KEY "Software\\Basis Technology\\RT Designer"
#else
 #define INITIAL_BLURB "Please try   File | Demo  for an animated demo of " PRODUCT_NAME "."
 #define NXSYS_LM_REG_KEY "Software\\B.Greenberg\\NXSYS"

#define APP_KEY NXSYS_LM_REG_KEY

#endif

#define MAIN_FRAME_SCREEN_X_FRACTION (1.0)
#ifdef NXV2
#define MAIN_FRAME_SCREEN_Y_FRACTION (10.5/16.0)
#else
#define MAIN_FRAME_SCREEN_Y_FRACTION (9.0/16.0)
#endif

#ifdef WIN32
   #ifdef RT_PRODUCT
   #define HELP_FNAME "rt-designer"
   #else
   #define HELP_FNAME "Pages\\NXSYS.html"
   #endif
#else
   #define HELP_FNAME "nxxlkg"
#endif

#define MAIN_WINDOW_STYLE \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL |\
  WS_MINIMIZEBOX | WS_MAXIMIZEBOX

BOOL No3DGraphics = FALSE;

/* should be moved to appropriate include files at some point */

void InitTrackGDI (int mww, int mwh);
void TrackCleanup(), TrackGraphicsCleanup ();
void TextFontCleanup();

void dealloc_lisp_sys();
void LispCleanOutRelays();

void CleanupObjectMemory();

#if defined(APPDEMO) | defined(RT_PRODUCT)
void DoV2Dialog();
#endif

#include "StartShut.h"

HFONT Fnt;
HFONT LargeFnt;
HWND G_mainwindow;
HINSTANCE app_instance;
HICON TrainMinimizedIcon;

std::string GlobalFilePathname;
#ifdef NXV2
#ifdef RT_PRODUCT
char IniFileName [] = "RTDESIGNER.INI";
char app_name [] = "RT Designer";
static char InitialTitleBar[] = "RT Designer -- New York-style NX/UR-style Panel";
#else
char IniFileName [] = "NXSYS.INI";
char app_name [] = "V2 NXSYS";
static char InitialTitleBar[] = "Version 2 NXSYS -- New York Subway NX/UR Panel";
#endif
#else
static char InitialTitleBar[] = "New York Subway NX/UR Panel";
char app_name [] = "NXSYS";
char IniFileName [] = "NXSYS.INI";

#endif

static char FName[MAXPATH] = "";


#ifndef NXSYSMac
static char MWPKey[] = "Main Window Placement";
static char FTitle[MAXPATH], DFName[MAXPATH] = "";
static char MainWindow_Class[] = PRODUCT_NAME ":Main";
#endif
char HelpPath[MAXPATH] = "Pages/NXSYS.html";

#ifdef NXOLE
static char ScriptName [MAXPATH];
#endif
static int  ChooseTrack = 0;
#ifndef NXSYSMac
static int  TrainType = 0;

BOOL RightButtonMenu = FALSE;
static BOOL AddedLastPath = FALSE;
#endif
static BOOL Got = 0;
static jmp_buf S_JmpBuf;

#ifdef NXSYSMac
void CloseAllFSWs(bool release);
void LoseChooseTrack();
#endif


static UINT OKNOICommands []
   = {(UINT)-1, CmOpen, CmQuit, CmDemo, CmDemoScript, CmCommandDlg};

#define NoOKNoICommands (sizeof(OKNOICommands)/sizeof(OKNOICommands[0]))

#ifdef NXOLE
static const char * initial_script_file = NULL;
static BOOL automation = FALSE;
#endif

static void CheckMainMenuItem (int item, BOOL enabled) {
    CheckMenuItem (GetMenu(G_mainwindow), item,
		   enabled ?
		   (MF_BYCOMMAND | MF_CHECKED) :
		   (MF_BYCOMMAND | MF_UNCHECKED));
}
    
static void SetGlobalMenuState (BOOL enable) {
    UINT enb = enable ? MF_ENABLED : MF_GRAYED;
    Got = enable;
    HMENU m = GetMenu (G_mainwindow);
    int lastmenu = 5;
#ifdef NXOGL
    lastmenu++;				/* cab view */
#endif
    if (m) {
	for (int i = 0; i < lastmenu; i++) { /* don't do HELP menu */
	    HMENU s = GetSubMenu (m, i);
	    int count = GetMenuItemCount (s);
	    for (int j = 0; j < count; j++) {
		UINT id = GetMenuItemID (s, j);
             int k;
		for (k = 0; k < NoOKNoICommands; k++)
		    if (id == OKNOICommands[k])
			break;
		if (k >= NoOKNoICommands)
		    EnableMenuItem (s, id, MF_BYCOMMAND | enb);
	    }
	}
    }
#ifdef NXV2
    if (AutoControlRelayExists()) {
	EnableMenuItem (m, CmAutoOp, MF_BYCOMMAND | MF_ENABLED);
	EnableAutomaticOperation (EnableAutoOperation);
    }
    else
	EnableMenuItem (m, CmAutoOp, MF_GRAYED);
#endif
}

#if WINDOWS
static void
SetHelpFilePath () {

    DWORD bufsize = sizeof(HelpPath), type;
    HKEY hKey;
    DWORD ec = RegOpenKeyEx
	       (HKEY_LOCAL_MACHINE, NXSYS_LM_REG_KEY, 0, KEY_READ, &hKey);
    if (ec == ERROR_SUCCESS) {
	ec = RegQueryValueEx(hKey, "HelpFilePathname",
			     NULL, &type, (PBYTE)HelpPath, &bufsize);
	RegCloseKey(hKey);
	if (ec == ERROR_SUCCESS)
	    return;
    }

    std::string drive, dir, name, ext;
    GetModuleFileName (app_instance, HelpPath, sizeof(HelpPath) - 1);
    STLfnsplit (HelpPath, drive, dir, name, ext);
   // fnmerge (std::string, drive, dir, HELP_FNAME, ".hlp");
	std::string hp1 = drive + dir + '\\' + HELP_FNAME
	hp1 = drive;
	hp1 += dir;
	hp1 += '\\';
	hp1 += HELP_FNAME;
	strcpy(HelpPath, hp1.c_str());
}
#endif

#ifdef WIN32
long smeasure (HDC dc, const char * str) {
    SIZE ss;
    GetTextExtentPoint32 (dc, str, strlen (str), &ss);
    return ss.cx;
}
#else
WORD smeasure (HDC dc, const char * str) {
    return LOWORD (GetTextExtent (dc, str, strlen (str)));
}
#endif


#if NXOGL
static BOOL NXGL_Loaded = FALSE;
static BOOL LayoutFedToNXGL = FALSE;
BOOL FeedLayoutToNXGL();

BOOL AutoLoadNXGL (BOOL barf) {
    if (No3DGraphics)
	return FALSE;
    NXGL_Loaded = NXGLMaybeInit(barf);
    if (NXGL_Loaded && !LayoutFedToNXGL && InterlockingLoaded) {
	LayoutFedToNXGL = FeedLayoutToNXGL();
    }
    return LayoutFedToNXGL;
}

#endif

void NBDSetWindowText (HWND window, const char* text) {
#if NXSYSMac
    SetWindowText(window, text);
#else
/* No, just plain SetWindowText fails pretty badly on Win95, leaving
   turds of previous message. */

    HDC dc = GetDC (window);
    RECT r;
    GetClientRect (window, &r);
/* won't look so good in 16-bit windows, needs more work there, but
   hell with it. ^&(^*%^&%^!! */
    HBRUSH b = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    SelectObject (dc, b);
    FillRect (dc, &r, b);
    ReleaseDC (window, dc);
    SendMessage (window, WM_SETTEXT, (WPARAM) 0, (LPARAM)(LPCSTR) text);
#endif
}

#if 0 /* not used in any case? */
/* Borrowed with permission from Windows RJ  (C) BSG 1993 */
static void GetWindowRectCl (HWND what, RECT &r, HWND parent) {

    RECT q;
    POINT p1, p2;
    GetWindowRect (what, &q);
    p1.x = q.left;
    p1.y = q.top;
    p2.y = q.bottom;
    p2.x = q.right;
    ScreenToClient (parent, &p1);
    ScreenToClient (parent, &p2);
	   
    r.left = p1.x;
    r.top = p1.y;
    r.right = p2.x;
    r.bottom = p2.y;
}
#endif

void SetViewportDimsFromWindow (HWND hWnd) {
    RECT r;
    GetClientRect (hWnd, &r);
    NXGO_SetViewportDims (r.right - r.left, r.bottom - r.top);
}

static void
InstallLayoutFile (HWND window, const char* s) {
#ifndef NXSYSMac // window not ready yet.  we mac do our own title, anyway
    char title[200];
    sprintf (title, "%s - %s", app_name, s);
    SetWindowText (window, title);
#endif
    ComputeVisibleObjectsLast();
    NXGO_ValidateWpVp(window);
    InvalidateRect (window, NULL, 1);
    UpdateWindow (window);
#ifdef NXOGL
    if (NXGL_Loaded && !No3DGraphics)
	LayoutFedToNXGL = FeedLayoutToNXGL();
#endif
}

void DeInstallLayout () {
#ifdef NXSYSMac
    CloseAllFSWs(true);  //should have something similar for MSWindows.
    LoseChooseTrack();
#endif
#ifndef NXSYSMac

    if (FName[0]) {
	char buf [MAX_PATH+10];
	sprintf (buf, "&1 %s", FName);
	HMENU menu = GetSubMenu (GetMenu (G_mainwindow),0);
	if (!AddedLastPath)
	    InsertMenu (menu, 2, MF_BYPOSITION, CmLoadLast, buf);
	else
	    ModifyMenu (menu, CmLoadLast, MF_BYCOMMAND, CmLoadLast, buf);
	AddedLastPath = TRUE;
    }
#endif
    /* loose trains are nasty. destroy them first. */
#ifndef NOTRAINS
    TrainMiscCtl (CmKillTrains);
#endif

    ClearHelpMenu();
    DraftsbeingCleanupForOneLayout();

#ifdef NXOGL
    NXGLUnloadLayout();
#endif

    KillNXTimers();
    DestroySigWins();

    DestroyDynMenus();
//   TrackCleanup();    //nxv1

    ValidateRelayWorld();
    FreeGraphicObjects();

    ValidateRelayWorld();
    CleanUpRelaySys();

    ValidateRelayWorld();
    LispCleanOutRelays();
    ValidateRelayWorld();
#ifndef NXV2
    dealloc_lisp_sys();			/* v2 will clobber syms... oops */
#endif
#ifdef NXCMPOBJ
    CleanupObjectMemory();
#endif
#if NXSYSMac
  //  MacReleaseGDIOBJs();   // why not reuse 'em?
#endif
    InvalidateRect(G_mainwindow, NULL, TRUE);
    Got = 0;
}


static void CallNormalSwitches (void*) {
    NormalAllSwitches();
}

void AllAbove() {
    if (!InterlockingLoaded)
	return;
    DropAllSignals();
    DropAllApproach();
    ClearAllTrackSecs();
#ifndef NOAUXK
    ClearAllAuxLevers();
#endif
    /* Have to wait for all stops to come up, and must process Win msgs */
    NXTimer (NULL, CallNormalSwitches, NORMAL_ALL_WAIT_TIME_MS);
}

BOOL GetLayout (const char * name, BOOL review) {
    ValidateRelayWorld();
    DeInstallLayout();
    if (review)
	SetViewportDimsFromWindow (G_mainwindow);
    WP_cord x, y;
    NXGO_GetDisplayWPOrg(x, y);
#if NXSYSMac
    MacBeforeLayoutLoad();
#endif
    const char * s = ReadLayout (name);
    Got = (s != NULL);
    SetGlobalMenuState (Got);

    if (Got) {
	if (!review)
	    NXGO_SetDisplayWPOrg(x, y);

        InstallLayoutFile (G_mainwindow, s);
        GlobalFilePathname = name;
#if NXSYSMac  // why isn't this crapola in AppDelegate?
        MacOnSuccessfulLayoutLoad(name);
#endif
    }
    if (Got) {
        
    }
    else
	DeInstallLayout();

    ValidateRelayWorld();
    return Got;
}
    

void EndChooseTrack () {
    ChooseTrack = 0;
    SetCursor (LoadCursor (NULL, IDC_ARROW));
    ReleaseCapture ();
    FlushChooseTrackDlg();
}
#ifndef NXSYSMac   // whole damned thing is worthless.

static void NXSYS_Command(unsigned int cmd) {
	int dlgr;
	switch (cmd) {
	case CmShowStops:
		if (dlgr = ShowStopDlg(G_mainwindow, app_instance, ShowStopPolicy)) {
			ImplementShowStopPolicy(dlgr);

			if (HKEY hk = GetAppKey("Settings")) {
				PutDWORDRegval(hk, "ShowStops", ShowStopPolicy);
				RegCloseKey(hk);
			}
			break;
#ifndef NOTRAINS
	case CmHaltTrains:
	case CmKillTrains:
	case CmHideTrainWindows:
	case CmShowTrainWindows:
		TrainMiscCtl(cmd);
		break;

	case CmNewTrain:
		TrainType = TRAIN_HALTCTL_GO;
	ctc:	    SetCapture(G_mainwindow);
		ChooseTrack = 1;
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		OfferChooseTrackDlg();
		break;
	case CmNewTrainStopped:
		TrainType = TRAIN_HALTCTL_HALTED;
		goto ctc;
#endif

	case CmResetAllAbove:
		AllAbove();
		break;

	case CmClearTrack:
		ClearAllTrackSecs();
		break;
	case CmCancelSignals:
		DropAllSignals();
		break;
	case CmReleaseApproach:
		DropAllApproach();
		break;
	case CmNormalSwitches:
		NormalAllSwitches();
		break;
	case CmBobbleRGPs:
		if (usermsggen(MB_YESNO,
			"Really simulate temporary failure "
			"of track repeater circuits?")
			== IDYES)
			BobbleRGPs();
		break;
#ifndef NOAUXK
	case CmClearAllAuxLevers:
		ClearAllAuxLevers();
		break;
#endif
	case CmRelayQuery:
		AskForAndShowStateRelay(G_mainwindow);
		break;

#ifndef NXSYSMac
	case CmRelayTrace:
		EnableRelayTrace(!RelayTraceExposedP());
		break;

	case CmNews:
		//obsolete 2016, no UI
		WinHelp(G_mainwindow, HelpPath, HELP_CONTEXT, IDH_NEWS);
	case CmUsage:
#ifdef WIN32
		ShellExecute(G_mainwindow, "open", HelpPath, NULL, NULL, SW_SHOWNORMAL);
#else
		WinHelp(G_mainwindow, HelpPath, HELP_CONTENTS, 0);
#endif
		break;
#endif
	case CmAbout:
		AboutDialog(G_mainwindow, app_instance);
		break;
	case CmLoadLast:
		if (!Got) {
			GetLayout(FName, TRUE);
			break;
		}
		/* otherwise fall through */
	case CmReload:
		if (Got && usermsggen(MB_YESNO, "Really reload and reset state?")
			== IDYES)
			GetLayout(FName, FALSE);
		break;
	case CmOpen:
		if (FileOpenDlg(G_mainwindow, FName, FTitle, sizeof(FName), 1,
#ifdef NXCMPOBJ
			FDE_Layout
#else
			FDE_Interpreted
#endif
		))
			GetLayout(FName, TRUE);
		break;
#ifndef	NODEMO
	case CmHaltDemo:
		DemoPause(1);
		break;
	case CmPauseDemo:
		DemoPause(0);
		break;
	case CmFlushSigWins:
		DestroySigWins();
		break;
	case CmDemo:
		if (FileOpenDlg(G_mainwindow, DFName, FTitle,
			sizeof(DFName), 1, FDE_XDO)) {
			AllAbove();
			Demo(DFName);
		}
		break;
#ifdef NXOLE
	case CmDemoScript:
		if (FileOpenDlg(G_mainwindow, ScriptName, FTitle,
			sizeof(ScriptName), 1, FDE_NXScript)) {
			AllAbove();
			NXScript(ScriptName);
		}
		break;
	case CmCommandDlg:
		CommandLoopDlg();
		break;
#endif
#endif
	case CmShowRelayCircuit:
		AskForAndDrawRelay(G_mainwindow);
		SetFocus(G_mainwindow);
		break;

	case CmPrintLogic:
		if (!InterlockingLoaded) {
			usermsgstop("No interlocking loaded.  Load it first.");
			break;
		}
		if (!InterpretedP) {
			usermsgstop("Compiled interlocking.  Load expr-code "
				"version or use Print Logic File.");
			break;
		}
		PrintInterlocking(InterlockingName);
		break;

	case CmPrintLogicFile:
		if (InterlockingLoaded)
			if (FileOpenDlg(G_mainwindow, DFName, FTitle, sizeof(DFName),
				1, FDE_Interpreted))
				DrawInterlockingFromFile(InterlockingName, DFName);
			else;
		else
			usermsgstop("No interlocking loaded.  Load it first.");
		break;

	case CmQuit:
#ifndef NXSYSMac
		PostQuitMessage(0);
#endif
		break;
	case CmScaleDisplay:
		if (ScaleDialog())
			InvalidateRect(G_mainwindow, NULL, TRUE);
		break;
#ifdef NXOGL

	case CmCabView:
		AutoLoadNXGL(TRUE);
		break;

	case CmUnloadCabView:
		NXGLUnload();
		NXGL_Loaded = LayoutFedToNXGL = FALSE;
		break;

	case CmAutoEngageTrains:
		AutoEngageTrains ^= 1;
		CheckMainMenuItem(CmAutoEngageTrains, AutoEngageTrains);
		break;
#endif		      
#ifdef NXV2
	case CmAutoOp:
		EnableAutoOperation = !EnableAutoOperation;
		CheckMainMenuItem(CmAutoOp, EnableAutoOperation);
		EnableAutomaticOperation(EnableAutoOperation);
		break;
#if defined(APPDEMO) | defined(RT_PRODUCT)
	case CmV2Info:
		DoV2Dialog();
		break;
#endif
#endif
	case CmScrollRight:
		NXGO_HScroll(G_mainwindow, SB_LINERIGHT, 0);
		break;
	case CmScrollLeft:
		NXGO_HScroll(G_mainwindow, SB_LINELEFT, 0);
		break;
	case CmScrollUp:
		NXGO_VScroll(G_mainwindow, SB_LINEUP, 0);
		break;
	case CmScrollDown:
		NXGO_VScroll(G_mainwindow, SB_LINEDOWN, 0);
		break;
	case CmPageUp:
		NXGO_VScroll(G_mainwindow, SB_PAGEUP, 0);
		break;
	case CmPageDown:
		NXGO_VScroll(G_mainwindow, SB_PAGEDOWN, 0);
		break;
	case CmHome:
		NXGO_HScroll(G_mainwindow, SB_TOP, 0);
		break;
	case CmEnd:
		NXGO_HScroll(G_mainwindow, SB_BOTTOM, 0);
		break;
	case CmPageLeft:
		NXGO_HScroll(G_mainwindow, SB_PAGELEFT, 0);
		break;
	case CmPageRight:
		NXGO_HScroll(G_mainwindow, SB_PAGERIGHT, 0);
		break;

	case CmRightClickMenuMode:
	{
#ifndef NXSYSMac
		RightButtonMenu = !RightButtonMenu;
		if (HKEY hk = GetAppKey("Settings")) {
			PutDWORDRegval(hk, "RightButtonMenuMode", RightButtonMenu);
			RegCloseKey(hk);
		}

#endif
		break;
	}

#ifndef NXSYSMac
#ifdef NXV2
	case CmV2NXHelp:
	{
		std::string V2HelpText;
		if (WinReadResText("V2NXHelp.txt", V2HelpText))
			HelpDialog(V2HelpText.c_str(), "Version 2 NXSYS Help");
		break;
	}
#endif
	default:
		if (cmd >= ID_EXTHELP0 && cmd <= ID_EXTHELP9)
			DisplayHelpTextByCommand(cmd);
		break;
#endif
		} // this must end the "switch"
	} //you would think that this would end the function, but ...

#endif

#ifndef NXSYSMac
} //  some conditionalizing problem
#endif

#ifdef EVALUATION_EDITION
static void ComplainExpired () {
    char BasisAddress[500];
    char Text[2000];
    LoadString (GetModuleHandle(NULL),
		IDS_BASIS_ADDRESS, BasisAddress, sizeof(BasisAddress));
    wsprintf (Text, "This evaluation copy of %s has expired.\r\n"
	      "For licensing information, please contact:\r\n\r\n%s",
	      PRODUCT_NAME, BasisAddress);
    MessageBox (0, Text, PRODUCT_NAME " Evaluation License",
		MB_ICONSTOP | MB_OK);
}
#endif


#ifndef NXSYSMac
WNDPROC_DCL MainWindow_WndProc
      (HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {

    case WM_COMMAND:

#ifdef EVALUATION_EDITION
	time_t exptime;
	if (time(&exptime) >= RTExpireTime) {
	    ComplainExpired();
	    SendMessage (window, WM_CLOSE, 0, 0);
	    return 0;
	}
#endif

      if (ChooseTrack)
	  EndChooseTrack ();

      NXSYS_Command (LOWORD(wParam));
      break;

    case WM_PAINT:
    {
	PAINTSTRUCT ps;
	HDC dc = BeginPaint (window, &ps);
	SelectObject (dc, Fnt);
	SetBkColor (dc, RGB(0, 0, 0));
	SetTextColor (dc, RGB(255,255,255));
	DisplayVisibleObjectsRect (dc, ps.rcPaint);
	EndPaint (window, &ps);
	break;
    }

#ifndef NXV2
    /* V2 trains can't possibly use track numbers.  There aren't any. */
    case WM_CHAR:
	wParam &= 0x7F;
	if (wParam >= '1' && wParam <= '9')
	    SendMessage (window, WM_NXSYS_SELTRACK, wParam-'0', 0L);
	break;

    case WM_NXSYS_SELTRACK:
	if (ChooseTrack) {
	    EndChooseTrack();
	    GraphicObject * g = ChooseTrackNumber (wParam);
	    if (g != NULL)
		TrainDialog (g, TrainType);
	}
	return 0;
#endif

    case WM_LBUTTONDOWN:
	if (wParam & MK_SHIFT)
	    message = WM_NXGO_LBUTTONSHIFT;
	else if (wParam & MK_CONTROL)
	    message = WM_NXGO_LBUTTONCONTROL;
    case WM_RBUTTONDOWN:
	if (message == WM_RBUTTONDOWN) {
	    if ((wParam & MK_CONTROL) || RightButtonMenu)
		message = WM_NXGO_RBUTTONCONTROL;
	}
#ifndef NOTRAINS
	if (ChooseTrack) {
	    EndChooseTrack ();
	    GraphicObject * g = FindHitObjectOfType
#ifdef NXV2
			  (ID_TRACKSEG, LOWORD (lParam), HIWORD (lParam));
#else
			  (ID_TRACKSEC, LOWORD (lParam), HIWORD (lParam));
#endif
	    if (g != NULL) {
		TrainDialog (g, TrainType);
		break;
	    }
	}
#endif
	NXGO_Rodentate (LOWORD (lParam), HIWORD (lParam), message);
	break;

    case WM_MOUSEMOVE:
	NXGOMouseMove (LOWORD (lParam), HIWORD (lParam));
	break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
	NXGOMouseUp ();
	break;

    case WM_SIZE:
	ResizeDemoWindowParent(window);

	SetViewportDimsFromWindow(window);

	if (InterlockingLoaded)
	    NXGO_ValidateWpVp(window);
	InvalidateRect (window, NULL, 1);
	break;
#ifndef NXSYSMac
    case WM_CLOSE:
	PostQuitMessage(0);
	break;

#endif
    case WM_HSCROLL:
	NXGO_HScroll (window, SCROLLARGS(wParam,lParam));
	break;

#ifdef NXV2
    case WM_VSCROLL:
	NXGO_VScroll (window, SCROLLARGS(wParam,lParam));
	break;
#endif

#ifndef NXSYSMac
      case WM_INITMENU:
    {
	HMENU m = GetMenu(window);
	CheckMenuItem (m, CmRightClickMenuMode,
		       MF_BYCOMMAND |
		       (RightButtonMenu  ? MF_CHECKED : MF_UNCHECKED));
       
	CheckMenuItem (m, CmRelayTrace,
		       MF_BYCOMMAND |
		       (RelayTraceExposedP() ? MF_CHECKED : MF_UNCHECKED));
	break;
    }
#endif
          
#if 0
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
	return NXGLForwardPalmsg (message, wParam);
#endif
#ifndef NXSYSMac
   default:
       	return DefWindowProc (window, message, wParam, lParam);
#endif
  }
  return 0;
       
}
    


WORD WindowsMessageLoop (HWND window, HACCEL hAccel, UINT closemsg) {
    MSG      message;

    while (GetMessage (&message, NULL, 0, 0)) {

	if (closemsg && message.message == closemsg)
	    break;

#ifdef NXV2
	if (IsMenuDlgMessage (&message))
	    continue;
#endif
#ifdef NXOLE
	if (IsCmdLoopDlgMessage (&message))
	    continue;
#endif
	
#ifndef NOTRAINS
	if (FilterTrainDialogMessages (&message))
	    continue;
#endif
	if (hAccel==NULL ||!TranslateAccelerator (window, hAccel, &message)){
	    TranslateMessage (&message);
	    DispatchMessage (&message);
	}
/*	HEAPCHK */
    }
    return message.wParam;
}
    
#endif

#ifndef NXSYSMac

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR command_line, int nCmdShow)
	{
  HWND     window;

#ifdef EVALUATION_EDITION
  long exptime;
  GetEncryptedTime(TimeCryptoKey, TimeCryptoLen, &exptime);
  RTExpireTime = exptime;
  if (time(&exptime) >= RTExpireTime) {
      ComplainExpired();
      return 0;
  }
#endif

#ifdef NXOGL
  BOOL UseNXGL = TRUE;
#endif

#ifndef WIN32
   if (hPrevInstance) {
       usermsgstop("Only one instance allowed at one time.");
       return 0;
   }
#endif



  app_instance = hInstance;
  const char * initial_layout_name = NULL;
#ifndef NODEMO
  const char * initial_demo_file = NULL;
#endif
  char ** argv;
  if (command_line)
      argv =  ParseArgString(command_line);
  else
      argv = NULL;
  int argc = ParseArgsArgCount (argv);
  for (int ano = 0; ano < argc; ano++) {
      const char * arg = argv[ano];
      if (arg[0] == '/' || arg[0] == '-') {
	  const char* argb = arg+1;
	  if (!_stricmp (argb, "no3dg"))
	      No3DGraphics = TRUE;
#ifndef NODEMO
	  else if (!_stricmp (argb, "demo")) {
	      if (ano >= argc - 1) {
			 usermsgstop ("Missing demo file name after %s", arg);
			 return 0;
	      }
	      initial_demo_file = argv[++ano];
	  }
#endif
	  else if (!_stricmp (argb, "contacts_per_line")) {
	      if (ano >= argc - 1) {
badcpl:		 usermsg ("Missing or bad number after -contacts_per_line arg.");
			 continue;
	      }
	      if (sscanf (argv[++ano], "%d", &ContactsPerLine) < 1)
			 goto badcpl;

	  }
	  else if (!_stricmp (argb, "automation") || !_stricmp (argb, "embedding"))
#ifdef NXOLE
	      automation = TRUE;
#else
	  {
	      usermsg ("Bad control arg: /%s; this executable of %s is not OLE-enabled.",
		       argb, PRODUCT_NAME);
	      return 0;
	  }
#endif
#ifdef NXOLE
	  else if (!_stricmp (argb, "register")) {
	      RegisterNXOLE();
	      return 0;
	  }
	  else if (!_stricmp (argb, "unregister")) {
	      UnRegisterNXOLE();
	      return 0;
	  }
	  else if (!_stricmp (argb, "script")) {
	      if (ano >= argc - 1) {
		  usermsgstop ("Missing script file name after %s", arg);
		  return 0;
	      }
	      initial_script_file = argv[++ano];
	  }
#endif
#ifdef NXOGL
	  else if (!strcmpi (argb, "nocab")) /* knio knio */
	      UseNXGL = FALSE;		/* no motorman cab view */
#endif
	  else
	      usermsg ("Bad/unknown control arg: %s", argv[ano]);
      }
      else initial_layout_name = arg;
  }


#ifndef NXSYSMac
    if (!hPrevInstance) {
      WNDCLASS klass;
      memset (&klass, 0, sizeof(klass));
      klass.style          = CS_BYTEALIGNCLIENT | CS_CLASSDC;
      klass.lpfnWndProc    = (WNDPROC) MainWindow_WndProc;
      klass.cbClsExtra     = 0;
      klass.cbWndExtra     = 10;
      klass.hInstance      = hInstance;
      klass.hIcon          = LoadIcon (hInstance, "NXICON");
      klass.hCursor        = LoadCursor (NULL, IDC_ARROW);
      klass.hbrBackground  = (HBRUSH)GetStockObject (BLACK_BRUSH);
      klass.lpszMenuName   = "NXSYS";
      klass.lpszClassName  = MainWindow_Class;

      if (!(RegisterClass (&klass)
	    && RegisterFullSignalWindowClass(hInstance)
	    && RegisterRelayTraceWindowClass (hInstance)
	    && RegisterRelayLogicWindowClass (hInstance)))
	  return 0;
  }
#endif
	HACCEL hAccel = LoadAccelerators(hInstance, "NXACC");
	window = NULL;   // KRAZY WTF NO WINDOW
    int rv = StartUpNXSYS (hInstance, window,
                           initial_layout_name, initial_demo_file, nCmdShow);
    if (rv == 0)
        rv = WindowsMessageLoop (G_mainwindow, hAccel, 0);
    

    ParseArgsFree(argv);
    CleanUpNXSYS();

    return rv;
}
#endif

int StartUpNXSYS (HINSTANCE hInstance, HWND window, const char * initial_layout_name, const char* initial_demo_file,
                  int nCmdShow) {

  TrainMinimizedIcon = LoadIcon (hInstance, "NXICON");

  RECT rc;
  GetWindowRect (GetDesktopWindow(), &rc);
  //int real_dtw = rc.right-rc.left;
  int real_dth = rc.bottom-rc.top;

/* 13 December 2000 -- setting the same metrics as those under which
   NXSYS was designed gives better results all-around -you
   get more track instead of thick tracks, and it seems to look better
   all around -- if you don't like it, scale. */

  int dtw = 800;
  int dth = 640;
#ifndef NXSYSMac
   int winy = dth/16;

   int winh =(int)(MAIN_FRAME_SCREEN_Y_FRACTION*dth);
   int winw = (int)(MAIN_FRAME_SCREEN_X_FRACTION*dtw);

   int winx = 200;
  winx = GetPrivateProfileInt(MWPKey, "UpperLeftX", winx, IniFileName);
  winy = GetPrivateProfileInt(MWPKey, "UpperLeftY", winy, IniFileName),
  winw = GetPrivateProfileInt(MWPKey, "Width", winw, IniFileName);
  winh = GetPrivateProfileInt(MWPKey, "Height", winh, IniFileName);

  DWORD style = MAIN_WINDOW_STYLE;
#ifdef NXV2
  style |= WS_VSCROLL;
#endif

  window = CreateWindow (MainWindow_Class,	/* class */
			 InitialTitleBar, style, winx, winy, winw, winh,
			 NULL, NULL, hInstance, NULL);
			  /* par, menu, instance, MDI */
  G_mainwindow = window;
#endif
  SetGlobalMenuState(FALSE);

  InitRelaySys();
  InitTrackGDI (dtw, dth);

#ifdef NXV2
  Glb.TorontoStyle = FALSE;
  Glb.AppBaseMajor = 2;
#else
  NXSYSGlobalDataInit();
#endif

  Glb.AppBaseMinor = NXSYS_APP_BASE_MINOR_VERSION;

#ifndef NODEMO
  CreateDemoWindow(window);
#endif
  LOGFONT lf;
  memset (&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = dth/60;

  /* 13 December 2000 -- needs to be a little bigger on bigger screen */
  if (real_dth > 640)
      lf.lfHeight+=2;

  strcpy (lf.lfFaceName, "Helvetica");
  Fnt = CreateFontIndirect (&lf);

  memset (&lf, 0, sizeof(LOGFONT));
  lf.lfWeight = FW_BOLD;
  lf.lfHeight = dth/35;
  strcpy (lf.lfFaceName, "Helvetica");
  LargeFnt = CreateFontIndirect (&lf);


#ifndef NXSYSMac
    if (HKEY hk = GetAppKey("Settings")) {
      RightButtonMenu = GetDWORDRegval (hk, "RightButtonMenuMode", RightButtonMenu);
      RegCloseKey(hk);
  }

  SetHelpFilePath();

#endif
    ShowWindow (window, nCmdShow);	/* don't gratuitously update */

#ifdef NXOGL
  if (UseNXGL) {
      CheckMainMenuItem (CmAutoEngageTrains, AutoEngageTrains);
      NXGL_Loaded = NXGLMaybeInit(FALSE);
  }
#endif
#ifdef NXV2
  CheckMainMenuItem(CmAutoOp, EnableAutoOperation);
#endif
  SetViewportDimsFromWindow (window);
#ifndef NXSYSMac
  if (HKEY shk = GetAppKey("Settings")) {
      ImplementShowStopPolicy(GetDWORDRegval(shk, "ShowStops", ShowStopPolicy));
      RegCloseKey(shk);
  }
#endif
#ifdef APPDEMO
  initial_layout_name = "000000.tko";
#endif

#ifndef NXSYSMac
  if (initial_layout_name) {
      strcpy (FName, initial_layout_name);
      const char * s = ReadLayout (initial_layout_name);
      if (s != NULL) {
	  SetGlobalMenuState(TRUE);
	  InstallLayoutFile(window, s);
      }
      else  {
	  DeInstallLayout();
      }
  }
#ifndef NODEMO
  else if (!initial_demo_file)
#ifdef NXOLE
      if (automation)
		 DemoBlurb (PRODUCT_NAME " via OLE automation!");
      else
		if (!initial_script_file)
#endif
#endif
             
#ifndef NXSYSMac // for now
	  DemoBlurb (INITIAL_BLURB);

#endif
  if (initial_demo_file)
      Demo (initial_demo_file);
#endif
#ifdef NXOLE
  InitNXOLE();

  if (initial_script_file)
      NXScript(initial_script_file);
#endif
#ifndef NXSYSMac
  HACCEL hAccel = LoadAccelerators (hInstance, "NXACC");
#endif
    int val = 0;  /* makes Apple logic analyzer happy */
	  
  switch (setjmp(S_JmpBuf)) {
      case 0:
	  val = 0;
	  break;
      case IDYES:
	  GetLayout (FName, FALSE);
	  val = 0;
	  break;
      case IDNO:
	  val = 99;
	  break;
      case IDCANCEL:
	  DeInstallLayout();
	  SetWindowText (G_mainwindow, InitialTitleBar);
	  InvalidateRect(G_mainwindow, NULL, TRUE);
	  val = 0;
	  break;
  };
  
    return val;
}


void CleanUpNXSYS() {
    
    DeleteObject (Fnt);
    DeleteObject (LargeFnt);
#ifndef NXSYSMac
	
	WinHelp(G_mainwindow, HelpPath, HELP_QUIT, 0);
#endif
    /* Why isn't DeInstallLayout good enough here? --11 January 2001 */
    ClearHelpMenu();
#ifdef NXOGL
    NXGLUnload();
#endif

#ifdef NXOLE
    CloseNXOLE();
#endif

    KillNXTimers();

    DraftsbeingCleanupForOneLayout();

    DraftsbeingCleanupForSystem(); //mac will clean up its own Cocoa window resources.

    DestroySigWins();
#ifdef NXV2
  DestroyDynMenus();
#endif
  
#ifndef NXV2
  TrackCleanup();
#endif
  TrackGraphicsCleanup();
  FreeGraphicObjects();
  TextFontCleanup();
  CleanUpRelaySys();
  dealloc_lisp_sys();
#ifdef NXCMPOBJ
  CleanupObjectMemory();
#endif

}


static int umsgcmn (va_list ap, const char * ctlstr, UINT ctl) {
    std::string msg = FormatStringVA(ctlstr, ap);
    return MessageBox (G_mainwindow, msg, app_name, ctl);
}

void usermsg (const char * ctlstr, ...) {
    va_list ap;
    va_start (ap, ctlstr);
    umsgcmn (ap, ctlstr, MB_OK | MB_ICONEXCLAMATION);
}

void usermsgstop (const char * ctlstr, ...) {
    va_list ap;
    va_start (ap, ctlstr);
    umsgcmn (ap, ctlstr, MB_OK | MB_ICONSTOP);
}

int usermsggen  (unsigned int flags, const char * ctlstr, ...) {
    va_list ap;
    va_start (ap, ctlstr);
    return umsgcmn (ap, ctlstr, flags);
}

#ifdef NXSYSMac
void ExternEditContextMenu(void * vobj, void* vmenu) {
    GraphicObject * g = (GraphicObject*) vobj;
    HMENU m = (HMENU)vmenu;
    g->EditContextMenu(m);
}
#endif

int GraphicObject::RunContextMenu (int resource_id) {

#if NXSYSMac
    return MacWindowsContextMenu(G_mainwindow, resource_id, this);
#else
    HMENU hMenu = LoadMenu(app_instance, MAKEINTRESOURCE(resource_id));
    if (!hMenu)
	return 0;
    POINT p;
    p.x = NXGOHitX;
    p.y = NXGOHitY;
    ClientToScreen (G_mainwindow, &p);
    HMENU m = GetSubMenu(hMenu, 0);
    EditContextMenu (m);
    int flgs = TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RETURNCMD;
    int cmd = TrackPopupMenu (m, flgs, p.x, p.y, 0, G_mainwindow, NULL);
    DeleteObject (hMenu);
    return cmd;
#endif
}


#if WINDOWS

int ContextMenu (int resource_id) {
    HMENU hMenu = LoadMenu(app_instance, MAKEINTRESOURCE(resource_id));
    if (!hMenu)
	return 0;
    POINT p;
    p.x = NXGOHitX;
    p.y = NXGOHitY;
    ClientToScreen (G_mainwindow, &p);
    HMENU m = GetSubMenu(hMenu, 0);
    int flgs = TPM_LEFTALIGN|TPM_TOPALIGN|TPM_NONOTIFY|TPM_RETURNCMD;
    int cmd = TrackPopupMenu (m, flgs, p.x, p.y, 0, G_mainwindow, NULL);
    DeleteObject (hMenu);
    return cmd;
}
#endif

void GraphicObject::EditContextMenu(HMENU m) {
}

#define NxsysAppAbortMsg \
 "A fatal relay logic error has occurred:\n   %s\n" \
 "Simulation of this scenario cannot proceed.\r\n" \
 "Do you want to reload and reinitialize this layout?\n" \
 "\n" \
 "  Click YES to reload and reinitialize this layout.\n" \
 "  Click NO to exit " PRODUCT_NAME ".\n" \
 "  Click CANCEL to return to " PRODUCT_NAME " with no layout loaded.\n"


void NxsysAppAbort (int reserved, const char* message) {
    std::string amessage = FormatString(NxsysAppAbortMsg, message);
    int val = MessageBox (G_mainwindow, amessage, PRODUCT_NAME " Layout fatal error",
                          MB_YESNOCANCEL | MB_ICONEXCLAMATION);
    throw nxterm_exception(val);
};

#if WINDOWS
HKEY GetAppKey(LPCSTR subk) {
    HKEY hk;
    char buf[MAX_PATH];
    strcpy (buf, APP_KEY);
    strcat (buf, "\\");
    strcat (buf, subk);
    if (ERROR_SUCCESS ==
	RegCreateKeyEx
	(HKEY_CURRENT_USER,
	 buf,
	 0,
	 NULL,
	 0,
	 KEY_ALL_ACCESS,
	 NULL,
	 &hk,
	 NULL))
	return hk;
    else
	return NULL;
}

	     

DWORD GetDWORDRegval (HKEY key, LPCSTR vname, DWORD val) {
    if (key){
	DWORD bufsize = sizeof(val);
	DWORD type;
	RegQueryValueEx(key, vname, NULL,
			&type, (PBYTE)&val, &bufsize);
    }
    return val;
}

DWORD PutDWORDRegval (HKEY key, LPCSTR vname, DWORD value) {
    if (key)
	RegSetValueEx(key ,vname ,0,
		      REG_DWORD, (LPBYTE)&value, sizeof(value));
    return value;
}
    
#endif
