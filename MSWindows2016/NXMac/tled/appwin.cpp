#include "windows.h"
#ifndef NXSYSMac
#include <commdlg.h>
#include <commctrl.h>
#endif
#include <string.h>
#include <stdio.h>
#ifndef NXSYSMac
#include <syserr32.h>
#include <stdarg.h>
#include <parsargs.h>
#endif
#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "tletoolb.h"
#include "tlecmds.h"
#ifndef NXSYSMac
#include "rubberbd.h"
#endif
#include "xtgtrack.h"
#include "tledit.h"
#include "objid.h"
#include "xtgload.h"
#include "lisp.h"
#include "dialogs.h"
#include "resource.h"
#include "assignid.h"
#include "dragger.h"
#include "objreg.h"
#include "signal.h"
#include <stdarg.h>

/* Global defined in this module */
HFONT Fnt = NULL;
int FixOriginWPX = 0, FixOriginWPY = 0;
HWND G_mainwindow = NULL, AppWindow = NULL;
HINSTANCE app_instance;
BOOL BufferModified = FALSE;
BOOL ExitLightsShowing = FALSE;
const char app_name[] = PRODUCT_NAME " Track Layout Editor";

#define EITHER_BUTTON (MK_LBUTTON | MK_RBUTTON)
#ifndef NXSYSMac
static HWND S_Statusbar = NULL;
#endif
static HWND S_Toolbar = NULL;

#ifdef NXSYSMac
void Mac_GetDisplayWPOrg(int[2], bool really_get_it_from_window);
void DisplayStatusString(const char * s);
void EnableCommand(UINT, bool);
#endif

#define TXWINSTYLE SS_SIMPLE | WS_CHILD | WS_VISIBLE
#define MAIN_FRAME_SCREEN_X_FRACTION (7.0/8.0)
#define MAIN_FRAME_SCREEN_Y_FRACTION (7.0/8.0)
#define APP_WIN_STYLE 	    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |\
			    WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX

static int MouseLastX = 0, MouseLastY = 0;
#ifndef NXSYSMac
static HCURSOR ChCursor = NULL;
static const char * IniFileName = "TLEDIT.INI";
static const char MWPKey[] = "Main Window Placement";
static const char InitialTitleBar[] = PRODUCT_NAME " Track Layout Editor";
static const char MainWindow_Class[] = PRODUCT_NAME ":TLEDITFrame";
static const char GraphicsWindow_Class[] = PRODUCT_NAME ":TLEDITGraphics";

static HCURSOR MMOOldCursor;
#endif

static char FileName[MAXPATH] = "";

static HFONT ComputeDefaultFont(int dth, int real_dth) {
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = dth / 60;
	strcpy(lf.lfFaceName, "Helvetica");

	/* 13 December 2000 -- needs to be a little bigger on bigger screen */
	if (real_dth > 640)
		lf.lfHeight += 2;

	return CreateFontIndirect(&lf);
}


#ifdef NXSYSMac
void SetMainWindowTitle(const char * s);
#else
static void SetMainWindowTitle(const char * s) {
	char buf[_MAX_PATH + 40];
	if (s)
		sprintf(buf, "TLEdit - %s", s);
	else
		strcpy(buf, "Track Layout Editor");
	SetWindowText(AppWindow, buf);
}
#endif

static int MapShowExitLight(GraphicObject * g) {
	((ExitLight *)g)->SetLit(ExitLightsShowing);
	return 0;
}

static void ShowExitLights(BOOL whichway) {
	ExitLightsShowing = whichway;
	MapGraphicObjectsOfType(ID_EXITLIGHT, MapShowExitLight);
}

static void GraphicsWindow_Rodentate
(HWND hWnd, int x, int y, UINT message, WORD mflags) {

	static int Captured = 0;

	if (message == WM_LBUTTONDOWN || message == WM_NXGO_LBUTTONCONTROL)
		mflags = MK_LBUTTON;
	else if (message == WM_RBUTTONDOWN)
		mflags = MK_RBUTTON;
	BOOL either = (mflags & EITHER_BUTTON) != 0;

	if (either) {
		if (!Captured) {
			Captured = 1;
#ifndef NXSYSMac
			SetCapture(hWnd);
#endif
		}
	}
	else {
		if (Captured) {
#ifndef NXSYSMac
			ReleaseCapture();
#endif
			Captured = 0;
		}
#ifndef NXSYSMac
		if (message == WM_MOUSEMOVE)
			return;
#endif
	}

	TrackLayoutRodentate(hWnd, message, x, y);
}


static int MapAssignIDs(GraphicObject * g) {
	TrackJoint * tj = (TrackJoint*)g;
	if (tj->Nomenclature)
		MarkIDAssign((int)(tj->Nomenclature));
	else
		if (tj->Insulated || tj->TSCount != 2)
			tj->Nomenclature = AssignID(1);
	if ((tj->Insulated || tj->TSCount != 2)
		&& tj->Nomenclature < TLEDIT_AUTO_ID_BASE)
		tj->PositionLabel();
	return 0;
}

static int MapAssignSigNos(GraphicObject * g) {
	PanelSignal * ps = (PanelSignal *)g;
	Signal * s = ps->Sig;
	if (s->XlkgNo)
		MarkIDAssign(s->XlkgNo);
	if (s->HeadsString == NULL)
		s->HeadsString = strdup("GYR");
	return 0;
}

static void FullRedisplay() {
	InvalidateRect(G_mainwindow, NULL, TRUE);
}

void FixOrigin(bool really_get_it_from_window) {
#ifdef NXSYSMac // gee, SCXtoWP ain't gonna work with NSScrollWindow . . .
	int coords[2];
	Mac_GetDisplayWPOrg(coords, really_get_it_from_window);
	FixOriginWPX = coords[0];
	FixOriginWPY = coords[1];
#else
	really_get_it_from_window;
	FixOriginWPX = (int)SCXtoWP(0);
	FixOriginWPY = (int)SCYtoWP(0);
#endif
}

void ClearItOut() {
	FreeGraphicObjects();
	strcpy(FileName, "");
	InitAssignID();
	BufferModified = FALSE;
}
static BOOL ReadIt() {
	FILE * f = fopen(FileName, "r");
	if (f == NULL) {
		usererr("Can't open %s for reading: %s", FileName,
#ifdef NXSYSMac
			strerror(NULL));
#else
			_strerror(NULL));
#endif
		return FALSE;
	}
	ClearItOut();
	if (XTGLoad(f)) {
		InitAssignID();
		MapGraphicObjectsOfType(ID_JOINT, MapAssignIDs);
		MapGraphicObjectsOfType(ID_SIGNAL, MapAssignSigNos);
		SetMainWindowTitle(FileName);
		ComputeVisibleObjectsLast();
		FullRedisplay();
		FixOrigin(false);
		fclose(f);
		return TRUE;
	}
	fclose(f);
	return FALSE;
}

const char * GetFileNamePointer() {
	return FileName;

}

BOOL SaveItForReal(const char * path) {
	if (SaveLayout(path)) {
		strcpy(FileName, path);
		BufferModified = FALSE;
		SetMainWindowTitle(FileName);
		StatusMessage("Wrote %s.", FileName);
		return TRUE;
	}
	return FALSE;
}

#ifndef NXSYSMac
static BOOL OpenIt() {
	if (FileOpenDlg(AppWindow, FileName, NULL, MAXPATH - 1, 1))
		return ReadIt();
	return FALSE;
}

static BOOL SaveIt(BOOL force_query) {
	if (!strcmp(FileName, ""))
		force_query = TRUE;
	if (force_query && !FileOpenDlg(AppWindow,
		FileName, NULL, MAXPATH - 1, 0))
		return FALSE;
	return SaveItForReal(FileName);
}

static BOOL CheckBufferModified() {
	if (!BufferModified)
		return TRUE;
	switch (MessageBox(AppWindow,
		"Layout has been modified.  Save it out?\r\n"
		"(Click \"No\" to discard the current image.)",
		app_name, MB_YESNOCANCEL | MB_ICONEXCLAMATION)) {
	case IDCANCEL:
		return FALSE;
	case IDYES:
		return SaveIt(FALSE);
	case IDNO:
		return TRUE;
	}
	return FALSE;
}

#endif
void AppCommand(UINT command) {
	switch (command) {
#ifndef NXSYSMac
	case CmQuit:
		if (CheckBufferModified())
			PostQuitMessage(0);
		break;

	case CmClear:
	case CmNew:
		if (CheckBufferModified()) {
			FreeGraphicObjects();
			FullRedisplay();
		}
		InitAssignID();
		BufferModified = FALSE;
		if (command == CmClear)
			SetMainWindowTitle(NULL);
		break;
#endif
	case CmRdis:
		FullRedisplay();
		break;
	case CmToggleExitLights:
		ExitLightsShowing = GetToolbarCheckState(S_Toolbar, command);
		if (SelectedObject && SelectedObject->TypeID() == ID_EXITLIGHT)
			SelectedObject->Deselect();
		ShowExitLights(ExitLightsShowing);
		break;
	case CmToggleNSJoints:
		ShowNonselectedJoints
			= GetToolbarCheckState(S_Toolbar, command);
		FullRedisplay();
		break;
	case CmIJ:
		if (SelectedObject && SelectedObject->TypeID() == ID_JOINT) {
			InsulateJoint((TrackJoint *)SelectedObject);
			//			    SelectedObject->Deselect();
		}
		break;
	case CmCut:
		if (SelectedObject)
			SelectedObject->Cut();
		break;
	case CmFlipSignal:
		if (SelectedObject)
			if (SelectedObject->TypeID() == ID_SIGNAL)
				FlipSignal((PanelSignal *)SelectedObject);
		break;
	case CmFlipNum:
		if (SelectedObject)
			if (SelectedObject->TypeID() == ID_JOINT)
				((TrackJoint *)SelectedObject)->FlipNum();
		break;

#ifndef NXSYSMac  // hic non est auxilium
	case CmHelp:
		DoHelpDialog();
		break;

	case CmAbout:
		DoAboutDialog();
		break;
#endif 
	case CmSignalUpRight:
	case CmSignalDownLeft:
		if (SelectedObject && SelectedObject->TypeID() == ID_JOINT)
			TLEditCreateSignal
			((TrackJoint *)SelectedObject,
				command == CmSignalUpRight);
		else if (SelectedObject && SelectedObject->TypeID() == ID_SIGNAL)
			TLEditCreateSignalFromSignal
			((PanelSignal *)SelectedObject,
				command == CmSignalUpRight);
		else
			usererr("An insulated joint must be selected to define a signal.");
		break;
	case CmCreateExitLight:
		if (SelectedObject && SelectedObject->TypeID() == ID_SIGNAL)
			TLEditCreateExitLightFromSignal
			((PanelSignal *)SelectedObject, TRUE);
		break;
	case CmSwitch:
		usererr("\"Define new switch\" not implemented yet.");
		break;
	case CmEditProperties:
		if (SelectedObject)
			SelectedObject->EditProperties();
		break;
#ifndef NXSYSMac
	case CmSaveAs:
		SaveIt(TRUE);
		break;
	case CmSave:
		SaveIt(FALSE);
		break;
	case CmOpen:
		if (CheckBufferModified())
			OpenIt();
		break;
#endif
	case CmRevertToSaved:
		if (!BufferModified)
			usererr("Drawing has not been modified, nothing to revert.");
		else if (IDOK == MessageBox
		(AppWindow, "Really discard this image and revert to saved?",
			app_name, MB_OKCANCEL | MB_ICONEXCLAMATION))
			ReadIt();
		break;
#ifndef NXSYSMac
	case IDM_SCALE:
		if (ScaleDialog())
			FullRedisplay();
		break;
	case IDM_SHIFT:
	{
		int x, y;
		if (DoShiftLayoutDlg(x, y)) {
			UpdateWindow(AppWindow);
			ShiftLayout(x, y);
			FullRedisplay();
		}
		break;
	}

#endif
	case IDM_FIX_ORIGIN:
		if (IDYES == MessageBox
		(G_mainwindow,
			"Do you really want to set the display origin"
			" as currently shown?",
			app_name, MB_YESNOCANCEL)) {
			FixOrigin(true);
			BufferModified = TRUE;
		}
		break;
	default:
		CreateObjectFromCommand(G_mainwindow, command, MouseLastX, 0);
		break;
	}
}

#ifndef NXSYSMac

static WNDPROC_DCL MainWindow_WndProc
(HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_COMMAND:

		if (MouseupDragon)
			MouseupDragon->Abort();

#ifdef EVALUATION_EDITION
		time_t exptime;
		if (time(&exptime) >= RTExpireTime) {
			ComplainExpired();
			SendMessage(window, WM_CLOSE, 0, 0);
			return 0;
		}
#endif

		AppCommand(LOWORD(wParam));
		break;

	case WM_SIZE:
		AutoResizeToolbar(S_Toolbar);
		if (G_mainwindow) {
			RECT r;
			RECT tbr;

			GetClientRect(window, &r);
			int width = r.right - r.left;
			int height = r.bottom - r.top;
			int top = 0;
			if (S_Toolbar) {
				GetWindowRect(S_Toolbar, &tbr);
				int tbh = tbr.bottom - tbr.top;
				height -= tbh;
				top += tbh;
			}
			if (S_Statusbar) {
				GetWindowRect(S_Statusbar, &tbr);
				int tbh = tbr.bottom - tbr.top;
				height -= tbh;
				MoveWindow(S_Statusbar, 0, top + height, width, tbh, 1);
			}
			MoveWindow(G_mainwindow, 0, top, width, height, 1);
		}
		return 0;
		break;

	case WM_NOTIFY:
		HandleToolbarNotification(wParam, lParam);
		break;
#ifndef NXSYSMac
	case WM_CLOSE:
		if (CheckBufferModified())
			PostQuitMessage(0);
		break;

	case WM_CHAR:
		if (MouseupDragon)
			MouseupDragon->Abort();
		break;

	default:
		return DefWindowProc(window, message, wParam, lParam);
#endif
	}
	return 0;

}
#endif


void EnableCommand(UINT cmd, BOOL way) {
	EnableToolButton(S_Toolbar, cmd, way);
	EnableMenuItem(GetMenu(AppWindow), cmd,
		MF_BYCOMMAND |
		(way ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
}

void GraphicsMouse(HWND window, unsigned message, WPARAM wParam, LPARAM lParam) {
	MouseLastX = (short)LOWORD(lParam);
	MouseLastY = (short)HIWORD(lParam);

	if (RodentatingDragon)
		RodentatingDragon->Rodentate
		(window, MouseLastX, MouseLastY, message, wParam);
	else
		GraphicsWindow_Rodentate		/* von WinRJ */
		(window, MouseLastX, MouseLastY, message, wParam);
}

#ifndef NXSYSMac
static WNDPROC_DCL GraphicsWindow_WndProc
(HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		GraphicsMouse(window, message, wParam, lParam);

		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(window, &ps);
		SelectObject(dc, Fnt);
		SetBkColor(dc, RGB(0, 0, 0));
		SetTextColor(dc, RGB(255, 255, 255));
		DisplayVisibleObjectsRect(dc, ps.rcPaint);
		EndPaint(window, &ps);
		break;
	}

	case WM_SIZE:
		NXGO_SetViewportDims(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_HSCROLL:
		NXGO_HScroll(window, LOWORD(wParam), HIWORD(wParam));
		break;
	case WM_VSCROLL:
		NXGO_VScroll(window, LOWORD(wParam), HIWORD(wParam));
		break;

	case WM_CHAR:
		if (MouseupDragon)
			MouseupDragon->Abort();
		break;

	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;

}


static long WindowsMessageLoop(HWND window, HACCEL hAccel) {
	MSG      message;

	while (GetMessage(&message, NULL, 0, 0)) {
		//	if (IsViewerDlgMsg (&message))
		//	    continue;

		BOOL was_mod = BufferModified;
		if (hAccel == NULL || !TranslateAccelerator(window, hAccel, &message)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		if (BufferModified != was_mod)
			EnableCommand(CmSave, BufferModified);
	}
	return message.wParam;
}
#endif

static void SelectHook(GraphicObject * go) {
	int objid = go ? go->TypeID() : -1;
	EnableCommand(CmCut, objid > 0);
	EnableCommand(CmEditProperties, objid > 0);
	EnableCommand(CmFlipNum, objid == ID_JOINT);
	EnableCommand(CmIJ, objid == ID_JOINT);
	EnableCommand(CmSignalUpRight, objid == ID_JOINT || objid == ID_SIGNAL);
	EnableCommand(CmSignalDownLeft, objid == ID_JOINT || objid == ID_SIGNAL);
	EnableCommand(CmCreateExitLight, objid == ID_JOINT || objid == ID_SIGNAL);
}

void InitTLEditApp(int dtw, int dth) {
	InitTrackGDI(dtw, dth);
	NXGO_SetSelectHook(SelectHook);
	SelectHook(NULL);
	EnableCommand(CmSave, FALSE);
#ifndef NXSYSMac
	RubberBandInit();
#endif
	InitLispSys();
	InitAssignID();
	InitializeRegisteredObjectClasses();
	XTGLoadInit();
#ifdef NXSYSMac
	Fnt = ComputeDefaultFont(640, 1280);
#endif
}

void CleanupTLEditApp() {
#ifndef NXSYSMac
	RubberBandClose();
#endif
	TrackGraphicsCleanup();
	FreeGraphicObjects();
	XTGLoadClose();
}

#ifndef NXSYSMac
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR command_line, int nCmdShow)
{

	WNDCLASS klass;
	app_instance = hInstance;

	char ** argv;
	if (command_line)
		argv = ParseArgString(command_line);
	else
		argv = NULL;
	int argc = ParseArgsArgCount(argv);

	if (!hPrevInstance) {

		if (!RegisterTextSampleClass(hInstance))
			return 0;

		memset(&klass, 0, sizeof(klass));
		klass.style = 0;
		klass.lpfnWndProc = (WNDPROC)MainWindow_WndProc;
		klass.hInstance = hInstance;
		klass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NXICON));
		klass.hCursor = LoadCursor(NULL, IDC_ARROW);
		klass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		klass.lpszMenuName = MAKEINTRESOURCE(IDM_MAIN_MENU);
		klass.lpszClassName = MainWindow_Class;

		if (!RegisterClass(&klass))
			return 0;

		memset(&klass, 0, sizeof(klass));
		klass.style = CS_BYTEALIGNCLIENT | CS_CLASSDC;
		klass.lpfnWndProc = (WNDPROC)GraphicsWindow_WndProc;
		klass.hInstance = hInstance;
		klass.hCursor = LoadCursor(NULL, IDC_ARROW);
		klass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		klass.lpszClassName = GraphicsWindow_Class;

		if (!RegisterClass(&klass))
			return 0;
	}

	RECT rc;
	GetWindowRect(GetDesktopWindow(), &rc);
	int real_dtw = rc.right - rc.left;
	int real_dth = rc.bottom - rc.top;

	/* 13 December 2000 -- setting the same metrics as those under which
	   NXSYS was designed gives better results all-around -you
	   get more track instead of thick tracks, and it seems to look better
	   all around -- if you don't like it, scale. */

	int dtw = 800;
	int dth = 640;

	int winx = (int)(dtw * (1.0 - MAIN_FRAME_SCREEN_X_FRACTION) / 2.0);
	int winy = (int)(dth * (1.0 - MAIN_FRAME_SCREEN_Y_FRACTION) / 2.0);
	int winw = (int)(dtw * MAIN_FRAME_SCREEN_X_FRACTION);
	int winh = (int)(dth * MAIN_FRAME_SCREEN_Y_FRACTION);

	if (dtw > 640)
		winw = dtw - winx;

	winx = GetPrivateProfileInt(MWPKey, "UpperLeftX", winx, IniFileName);
	winy = GetPrivateProfileInt(MWPKey, "UpperLeftY", winy, IniFileName),
		winw = GetPrivateProfileInt(MWPKey, "Width", winw, IniFileName);
	winh = GetPrivateProfileInt(MWPKey, "Height", winh, IniFileName);

	AppWindow = CreateWindow(MainWindow_Class, InitialTitleBar, APP_WIN_STYLE,
		winx, winy, winw, winh,
		NULL, NULL, hInstance, NULL);

	Fnt = ComputeDefaultFont(dth, real_dth);

	G_mainwindow = CreateWindow(GraphicsWindow_Class,
		NULL, WS_CHILD | WS_HSCROLL | WS_VSCROLL,
		0, 0, 100, 100,
		AppWindow, NULL, hInstance, NULL);

	S_Toolbar = CreateOurToolbar(AppWindow, hInstance);
	if (S_Toolbar) {
		SetToolbarCheckState(S_Toolbar, CmToggleNSJoints, TRUE);
		AssertToolbarCheckState(S_Toolbar, CmPoint);
		ShowWindow(S_Toolbar, SW_SHOW);
		S_Statusbar = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
			"Type F1 for Help.",
			AppWindow, 777);
	}

	if (!(S_Statusbar && S_Toolbar)) {
		usererr("Cannot create statusbar/toolbar: %s", SysLastErrstr());
		DestroyWindow(AppWindow);
		return 0;
	}

	ChCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDCR_CROSSHAIR));
	InitTLEditApp(dtw, dth);

	ShowWindow(G_mainwindow, SW_SHOW);

	StatusMessage("Press F1 for help.");

	ShowWindow(AppWindow, nCmdShow);	/* don't gratuitously update */

	if (argc >= 1) {
		strcpy(FileName, argv[0]);
		ReadIt();
	}

	int val = WindowsMessageLoop
	(AppWindow,
		LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TLEDIT)));
	CleanupTLEditApp();
	ParseArgsFree(argv);  return val;
	return val;

}
#endif

void usererr(const char * control_string, ...) {
	char buf[300];
	va_list arg_ptr;
	va_start(arg_ptr, control_string);
	vsprintf(buf, control_string, arg_ptr);
	va_end(arg_ptr);
	MessageBox(G_mainwindow, buf, app_name, MB_OK);
}

void StatusMessage(const char * control_string, ...) {
	char buf[400];
	va_list arg_ptr;
	va_start(arg_ptr, control_string);
	vsprintf(buf, control_string, arg_ptr);
	va_end(arg_ptr);
#ifdef NXSYSMac
	DisplayStatusString(buf);
#else
	SetWindowText(S_Statusbar, buf);
#endif
}

void SetViewportDimsFromWindow(HWND hWnd) {
	RECT r;
	GetClientRect(hWnd, &r);
	NXGO_SetViewportDims(r.right - r.left, r.bottom - r.top);
}

#ifdef NXSYSMac
BOOL ReadItKludge(const char * filename) {
	strcpy(FileName, filename);
	return ReadIt();
}
#endif