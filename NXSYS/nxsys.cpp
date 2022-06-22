#define NXSYS_APP_BASE_MINOR_VERSION 3

#include "windows.h"
#include <string>
#include <vector>
#include <utility>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "nxsysapp.h"
#include "commands.h"
#include "nxgo.h"
#include "timers.h"
#include "typeid.h"
#include "compat32.h"
#include "lyglobal.h"
#include "loaddcls.h"
#include "xtgload.h"
#include "xtgtrack.h"
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
#include "replace_filename.h"
#include "STLExtensions.h"
#include "WinApiSTL.h"
#include "AppAbortRestart.h"
#include "MessageBox.h"

#include <filesystem>

namespace fs = std::filesystem;
using std::string;
using std::vector;

#ifdef NXSYSMac
#include "AppDelegateGlobals.h"
#endif

#ifdef WIN32
#include "NXRegistry.h"
#include "LDRightClick.h"
#include "NXSYSWinApp.h"
#include "RecentFileMan.h"

#endif

#ifdef NXOLE
#include "nxole.h"
#endif

#include "trainapi.h"
#include "dynmenu.h"
BOOL EnableAutoOperation = FALSE;

void ValidateRelayWorld();

/* time to wait for Normal All Switches */
#define NORMAL_ALL_WAIT_TIME_MS 1500L

#define INITIAL_BLURB "Please try   File | Demo  for an animated demo of " PRODUCT_NAME "."

#define MAIN_FRAME_SCREEN_X_FRACTION (1.0)
#define MAIN_FRAME_SCREEN_Y_FRACTION (10.5/16.0)

#define HELP_FNAME "Documentation\\NXSYS.html"

#define MAIN_WINDOW_STYLE \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL |\
  WS_MINIMIZEBOX | WS_MAXIMIZEBOX

/* should be moved to appropriate include files at some point */

void InitTrackGDI (int mww, int mwh);
void TrackCleanup(), TrackGraphicsCleanup ();
void TextFontCleanup();
void dealloc_lisp_sys();
void LispCleanOutRelays();
void CleanupObjectMemory();

#include "StartShut.h"

HFONT Fnt;
HFONT LargeFnt;
HWND G_mainwindow;
HINSTANCE app_instance;
HICON TrainMinimizedIcon;

string GlobalFilePathname;

char app_name[] = "V2 NXSYS";
string LayoutFileName;

static const char* InitialTitleBar = "Version 2 NXSYS -- New York Subway NX/UR Panel"
#ifdef _WIN64
  " (64-bit)"
#endif
#if defined(_DEBUG) || defined(DEBUG)
  " (DEBUG)"
#endif
   ;

#ifndef NXSYSMac
static char MWPKey[] = "Main Window Placement";
static const char* IniFileName = "NXSYS.INI";
#endif

#ifdef NXOLE
static char ScriptName [MAXPATH];
#endif
int  ChooseTrackActive = 0;
#ifndef NXSYSMac
static int  TrainType = 0;

BOOL RightButtonMenu = FALSE;
static BOOL AddedLastPath = FALSE;
#endif
BOOL GotLayout = 0;
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
    
static void SetGlobalMenuState (BOOL enable) {
    UINT enb = enable ? MF_ENABLED : MF_GRAYED;
    GotLayout = enable;
    HMENU m = GetMenu (G_mainwindow);
    int lastmenu = 5;

	if (m) {
		for (int i = 0; i < lastmenu; i++) { /* don't do HELP menu */
			HMENU s = GetSubMenu(m, i);
			int count = GetMenuItemCount(s);
			for (int j = 0; j < count; j++) {
				UINT id = GetMenuItemID(s, j);
				int k;
				for (k = 0; k < NoOKNoICommands; k++)
					if (id == OKNOICommands[k])
						break;
				if (k >= NoOKNoICommands)
					EnableMenuItem(s, id, MF_BYCOMMAND | enb);
			}
		}
	}
	if (AutoControlRelayExists()) {
		EnableMenuItem(m, CmAutoOp, MF_BYCOMMAND | MF_ENABLED);
		EnableAutomaticOperation(EnableAutoOperation);
	}
	else
		EnableMenuItem(m, CmAutoOp, MF_GRAYED);
}

#if WIN32
long smeasure (HDC dc, const char * str) {
    SIZE ss;
    GetTextExtentPoint32 (dc, str, (int)strlen (str), &ss);
    return ss.cx;
}
#else
WORD smeasure (HDC dc, const char * str) {
    return LOWORD (GetTextExtent (dc, str, strlen (str)));
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
	RECT r{};
    GetClientRect (hWnd, &r);
    NXGO_SetViewportDims (r.right - r.left, r.bottom - r.top);
}

static void
InstallLayoutFile (HWND window, const char* layout_name) {
#ifndef NXSYSMac // window not ready yet.  we mac do our own title, anyway
	SetWindowTextS(window, string(app_name) + " - " + layout_name);
#endif
    ComputeVisibleObjectsLast();
    NXGO_ValidateWpVp(window);
    InvalidateRect (window, NULL, 1);
    UpdateWindow (window);
}

void DeInstallLayout () {
#ifdef NXSYSMac
    CloseAllFSWs(true);  //should have something similar for MSWindows.
    LoseChooseTrack();
#else

    if (LayoutFileName.length()) {
		std::string entry = "&1 " + LayoutFileName;	
		HMENU menu = GetSubMenu (GetMenu (G_mainwindow),0);
		if (!AddedLastPath)
		  InsertMenu (menu, 2, MF_BYPOSITION, CmLoadLast, entry.c_str());
		else
		   ModifyMenu (menu, CmLoadLast, MF_BYCOMMAND, CmLoadLast, entry.c_str());
		AddedLastPath = TRUE;
    }
#endif
    /* loose trains are nasty. destroy them first. */
    TrainMiscCtl (CmKillTrains);

    ClearHelpMenu();
    DraftsbeingCleanupForOneLayout();

    KillNXTimers();
    DestroySigWins();

    DestroyDynMenus();
#if !TLEDIT
	TrackCircuitSystemReInit();
#endif
    ValidateRelayWorld();
    FreeGraphicObjects();

    ValidateRelayWorld();
    CleanUpRelaySys();

    ValidateRelayWorld();
    LispCleanOutRelays();
    ValidateRelayWorld();

#ifdef NXCMPOBJ
    CleanupObjectMemory();
#endif
#if NXSYSMac
  //  MacReleaseGDIOBJs();   // why not reuse 'em?
#endif
    InvalidateRect(G_mainwindow, NULL, TRUE);
    GotLayout = 0;
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
    ClearAllAuxLevers();
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
    GotLayout = (s != NULL);
    SetGlobalMenuState (GotLayout);

    if (GotLayout) {
	if (!review)
	    NXGO_SetDisplayWPOrg(x, y);

        InstallLayoutFile (G_mainwindow, s);
        GlobalFilePathname = name;
#if NXSYSMac  // why isn't this crapola in AppDelegate?
        MacOnSuccessfulLayoutLoad(name);
#else
		AssertRecentFileUse(name);
#endif
    }
    if (!GotLayout)
		DeInstallLayout();

    ValidateRelayWorld();
    return GotLayout;
}
    

void EndChooseTrack () {
    ChooseTrackActive = 0;
    SetCursor (LoadCursor (NULL, IDC_ARROW));
    ReleaseCapture ();
    FlushChooseTrackDlg();
}

void CheckMainMenuItem2(int item, BOOL enabled) { //yes, this works on the Mac....
    CheckMenuItem(GetMenu(G_mainwindow), item,
        enabled ?
        (MF_BYCOMMAND | MF_CHECKED) :
        (MF_BYCOMMAND | MF_UNCHECKED));
}

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

#ifdef WIN32
  int winy = dth / 16;

  int winh = (int)(MAIN_FRAME_SCREEN_Y_FRACTION * dth);
  int winw = (int)(MAIN_FRAME_SCREEN_X_FRACTION * dtw);

  int winx = 200;
  winx = GetPrivateProfileInt(MWPKey, "UpperLeftX", winx, IniFileName);
  winy = GetPrivateProfileInt(MWPKey, "UpperLeftY", winy, IniFileName),
  winw = GetPrivateProfileInt(MWPKey, "Width", winw, IniFileName);
  winh = GetPrivateProfileInt(MWPKey, "Height", winh, IniFileName);

  DWORD style = MAIN_WINDOW_STYLE;
  style |= WS_VSCROLL;

  window = CreateWindow (MainWindow_Class,	/* class */
			 InitialTitleBar, style, winx, winy, winw, winh,
			 NULL, NULL, hInstance, NULL);
			  /* par, menu, instance, MDI */
  G_mainwindow = window;
  InitRelayGraphicsSourceClick();
#endif
  SetGlobalMenuState(FALSE);

  InitRelaySys();
  InitTrackGDI (dtw, dth);
  XTGLoadInit();

  Glb.TorontoStyle = FALSE;
  Glb.AppBaseMajor = 2;
  Glb.AppBaseMinor = NXSYS_APP_BASE_MINOR_VERSION;

  CreateDemoWindow(window);

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
  AppKey hk("Settings");
  if (hk)
      RightButtonMenu = GetDWORDRegval (hk, "RightButtonMenuMode", RightButtonMenu);

#endif

  
  ShowWindow (window, nCmdShow);	/* don't gratuitously update */
#ifdef WIN32
  CheckMainMenuItem2(CmAutoOp, EnableAutoOperation);
#endif
  SetViewportDimsFromWindow (window);
#ifndef NXSYSMac
  if (hk) 
      ImplementShowStopPolicy(GetDWORDRegval(hk, "ShowStops", ShowStopPolicy));

  HMENU top_level_menu = GetMenu(G_mainwindow),
        file_menu = GetSubMenu(top_level_menu, 0),
        recent_files_submenu = GetSubMenu(file_menu, 3);
  InitMenuRecentFiles(recent_files_submenu);

  if (initial_layout_name) {
	  LayoutFileName = initial_layout_name;
      const char * s = ReadLayout (initial_layout_name);
      if (s != NULL) {
		  SetGlobalMenuState(TRUE);
		  AssertRecentFileUse(initial_layout_name);
		  InstallLayoutFile(window, s);
      }
      else  {
		 DeInstallLayout();
      }
  }

  else if (!initial_demo_file)
#ifdef NXOLE
      if (automation)
		 DemoBlurb (PRODUCT_NAME " via OLE automation!");
      else
		if (!initial_script_file)
#endif
             
#ifdef WIN32 // for now
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
	  GetLayout (LayoutFileName.c_str(), FALSE);
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

	DeleteObject(Fnt);
	DeleteObject(LargeFnt);
	/* Why isn't DeInstallLayout good enough here? --11 January 2001 */
	ClearHelpMenu();

#ifdef NXOLE
	CloseNXOLE();
#endif

	KillNXTimers();

	DraftsbeingCleanupForOneLayout();
	DraftsbeingCleanupForSystem(); //mac will clean up its own Cocoa window resources.

	DestroySigWins();
	DestroyDynMenus();
	TrackGraphicsCleanup();
	FreeGraphicObjects();
	TextFontCleanup();
	CleanUpRelaySys();
	dealloc_lisp_sys();
#ifdef NXCMPOBJ
	CleanupObjectMemory();
#endif

}


static int umsgcmn (va_list ap, const char* ctlstr, UINT ctl) {
    std::string msg = FormatStringVA(ctlstr, ap);
    return MessageBoxS (G_mainwindow, msg, app_name, ctl);
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

#ifdef NXSYSMac
    return MacWindowsContextMenu(G_mainwindow, resource_id, this);
#else
    HMENU hMenu = LoadMenu(app_instance, MAKEINTRESOURCE(resource_id));
    if (!hMenu)
	return 0;
	POINT p{};
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
    int val = MessageBox (G_mainwindow, amessage.c_str(), PRODUCT_NAME " Layout fatal error",
                          MB_YESNOCANCEL | MB_ICONEXCLAMATION);
    throw nxterm_exception(val);
};


