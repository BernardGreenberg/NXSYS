#include <windows.h>

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
#include "trainapi.h"
#include "resource.h"
#include <string>
#include "incexppt.h"
#include "STLExtensions.h"
#include "WinApiSTL.h"
#include "STLfnsplit.h"
#include "AppAbortRestart.h"
#include "MessageBox.h"
#include "NXRegistry.h"
#include "NXSYSWinApp.h"
#include "WinReadResText.h"
#include "RecentFileMan.h"
#include "ParseCommandLine.h"
#include "GetResourceDirectoryPathname.h"
#include "InterlockingLibrary.hpp"
#include "HelpDirectory.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using std::string;
using std::vector;

static int TrainType;
static char FTitle[MAXPATH], DFName[MAXPATH] = "";
static InterlockingLibrary Library;
static HelpDirectory Helps;

const char* MainWindow_Class = PRODUCT_NAME ":Main";
#define HELP_FNAME "Documentation\\NXSYS.html"

void CheckMainMenuItem(int item, BOOL enabled) {
	CheckMenuItem(GetMenu(G_mainwindow), item,
		enabled ?
		(MF_BYCOMMAND | MF_CHECKED) :
		(MF_BYCOMMAND | MF_UNCHECKED));
}

void NXSYS_Command(unsigned int cmd) {
	int dlgr;
	switch (cmd) {
	case CmShowStops:
		if (dlgr = ShowStopDlg(G_mainwindow, app_instance, ShowStopPolicy)) {
			ImplementShowStopPolicy(dlgr);
			AppKey hk("Settings");
			if (hk)
				PutDWORDRegval(hk, "ShowStops", ShowStopPolicy);
		}
		break;
            
    /* of trains ... */
	case CmHaltTrains:
	case CmKillTrains:
	case CmHideTrainWindows:
	case CmShowTrainWindows:
		TrainMiscCtl(cmd);
		break;

	case CmNewTrain:
		TrainType = TRAIN_HALTCTL_GO;
	ctc:	    SetCapture(G_mainwindow);
		ChooseTrackActive = 1;
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		OfferChooseTrackDlg();
		break;
	case CmNewTrainStopped:
		TrainType = TRAIN_HALTCTL_HALTED;
		goto ctc;

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

	case CmClearAllAuxLevers:
		ClearAllAuxLevers();
		break;

	case CmRelayQuery:
		AskForAndShowStateRelay(G_mainwindow);
		break;

#ifndef NXSYSMac
	case CmRelayTrace:
		EnableRelayTrace(!RelayTraceExposedP());
		break;

	case CmNews:
		//obsolete 2016, no UI
		//WinHelp(G_mainwindow, HelpPath.c_str(), HELP_CONTEXT, IDH_NEWS);
	case CmUsage:
	{
		std::vector<char>buf(MAX_PATH);
		GetModuleFileName(app_instance, buf.data(), (WORD)buf.size() - 1);
		fs::path modpath = string(buf.data());
		modpath.replace_filename("");
		string path = (fs::path(modpath / fs::path(HELP_FNAME))).string();
		WinBrowseResource(path.c_str());
		break;
	}
	case CmAbout:
		AboutDialog(G_mainwindow, app_instance);
		break;
	case CmLoadLast:
		if (!GotLayout) {
			GetLayout(LayoutFileName.c_str(), TRUE);
			break;
		}
	case CmReload:
		if (GotLayout && usermsggen(MB_YESNO, "Really reload and reset state?")
			== IDYES)
			GetLayout(LayoutFileName.c_str(), FALSE);
		break;
	case CmOpen:
	{
		auto result = FileOpenDlgSTL(G_mainwindow, LayoutFileName, FTitle, true,
#ifdef NXCMPOBJ
			FDlgExt::Layout
#else
			FDlgExt::Interpreted
#endif
		);
		if (result.valid) {
			LayoutFileName = result.Path;
			GetLayout(LayoutFileName.c_str(), TRUE);
		}
	}
	break;

	case CmFileInfo:
		if (InterlockingLoaded)
			StatusReportDialog(G_mainwindow, GetModuleHandle(NULL));
		break;

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
	{
		auto result = FileOpenDlgSTL(G_mainwindow, "", "Demo script", true, FDlgExt::XDO);
		if (result.valid) {}
		AllAbove();
		Demo(result.Path.c_str());
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
		PrintInterlocking(InterlockingName.c_str());
		break;

	case CmPrintLogicFile:
		if (InterlockingLoaded)
			if (FileOpenDlg(G_mainwindow, DFName, FTitle, sizeof(DFName),
				1, FDlgExt::Interpreted))
				DrawInterlockingFromFile(InterlockingName.c_str(), DFName);
			else;
		else
			usermsgstop("No interlocking loaded.  Load it first.");
		break;

	case CmQuit:

		PostQuitMessage(0);
		break;

	case CmScaleDisplay:
		if (ScaleDialog())
			InvalidateRect(G_mainwindow, NULL, TRUE);
		break;

	case CmAutoOp:
		EnableAutoOperation = !EnableAutoOperation;
		CheckMainMenuItem(CmAutoOp, EnableAutoOperation);
		EnableAutomaticOperation(EnableAutoOperation);
		break;

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

		RightButtonMenu = !RightButtonMenu;
		{
			AppKey hk("Settings");
			if (hk)
				PutDWORDRegval(hk, "RightButtonMenuMode", RightButtonMenu);
		}
		break;


	case CmV2NXHelp:
	{
		std::string V2HelpText;
		if (WinReadResText("V2NXHelp.txt", V2HelpText))
			HelpDialog(V2HelpText.c_str(), "Version 2 NXSYS Help");
		break;
	}
	default:
		if (cmd >= ID_EXTHELP0 && cmd <= ID_EXTHELP9) {
			DisplayHelpTextByCommand(cmd);
			break;
		}
		if (cmd >= ID_LIBRARY_BASE && cmd - ID_LIBRARY_BASE < Library.size()) {
			GetLayout(Library[cmd - ID_LIBRARY_BASE].Pathname.string().c_str(), TRUE);
			break;
		}
		if (cmd >= ID_HELP_BASE && cmd - ID_HELP_BASE <= Helps.size()) {
			WinBrowseResource(Helps[cmd - ID_HELP_BASE].URL.c_str());
			break;
		}
		auto rflresult = HandleRecentFileClick(cmd);
		if (rflresult.valid) {
			GetLayout(rflresult.pathname.c_str(), TRUE);
			break;
		}
		break;

	} // this must end the "switch"
}

#endif
WNDPROC_DCL MainWindow_WndProc
(HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

	case WM_COMMAND:

		if (ChooseTrackActive)
			EndChooseTrack();

		NXSYS_Command(LOWORD(wParam));
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
		if (ChooseTrackActive) {
			EndChooseTrack();
			GraphicObject* g = FindHitObjectOfType
			(ID_TRACKSEG, LOWORD(lParam), HIWORD(lParam));
			if (g != NULL) {
				TrainDialog(g, TrainType);
				break;
			}
		}
		NXGO_Rodentate(LOWORD(lParam), HIWORD(lParam), message);
		break;

	case WM_MOUSEMOVE:
		NXGOMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		NXGOMouseUp();
		break;

	case WM_SIZE:
		ResizeDemoWindowParent(window);

		SetViewportDimsFromWindow(window);

		if (InterlockingLoaded)
			NXGO_ValidateWpVp(window);
		InvalidateRect(window, NULL, 1);
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_HSCROLL:
		NXGO_HScroll(window, SCROLLARGS(wParam, lParam));
		break;

	case WM_VSCROLL:
		NXGO_VScroll(window, SCROLLARGS(wParam, lParam));
		break;

	case WM_INITMENU:
	{
		HMENU m = GetMenu(window);
		CheckMenuItem(m, CmRightClickMenuMode,
			MF_BYCOMMAND |
			(RightButtonMenu ? MF_CHECKED : MF_UNCHECKED));

		CheckMenuItem(m, CmRelayTrace,
			MF_BYCOMMAND |
			(RelayTraceExposedP() ? MF_CHECKED : MF_UNCHECKED));
		break;
	}


	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;

}


WORD WindowsMessageLoop(HWND window, HACCEL hAccel, UINT closemsg) {
	MSG      message;

	while (GetMessage(&message, NULL, 0, 0)) {

		if (closemsg && message.message == closemsg)
			break;

		if (IsMenuDlgMessage(&message))
			continue;

#ifdef NXOLE
		if (IsCmdLoopDlgMessage(&message))
			continue;
#endif
		if (FilterTrainDialogMessages(&message))
			continue;
		if (hAccel == NULL || !TranslateAccelerator(window, hAccel, &message)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		/*	HEAPCHK */
	}
	return (WORD)message.wParam;
}

int ContextMenu(int resource_id) {
	HMENU hMenu = LoadMenu(app_instance, MAKEINTRESOURCE(resource_id));
	if (!hMenu)
		return 0;
	POINT p{};
	p.x = NXGOHitX;
	p.y = NXGOHitY;
	ClientToScreen(G_mainwindow, &p);
	HMENU m = GetSubMenu(hMenu, 0);
	int flgs = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD;
	int cmd = TrackPopupMenu(m, flgs, p.x, p.y, 0, G_mainwindow, NULL);
	DeleteObject(hMenu);
	return cmd;
}

static void SetUpLibraryMenu() {
	Library = GetInterlockingLibrary();
	int i = 0;
	HMENU main_menu = GetMenu(G_mainwindow),
		file_menu = GetSubMenu(main_menu, 0),
		lib_menu = GetSubMenu(file_menu, 2);
	DeleteMenu(lib_menu, ID_1DUMY, MF_BYCOMMAND);
	for (auto& libe : Library)
		InsertMenu(lib_menu, -1, MF_BYPOSITION, ID_LIBRARY_BASE + i++, libe.Title.c_str());

}
static void SetUpHelpMenu() {
	Helps = GetHelpDirectory();
	int i = 0;
	HMENU main_menu = GetMenu(G_mainwindow),
		help_menu = GetSubMenu(main_menu, 5);
	DeleteMenu(help_menu, ID_1DUMY, MF_BYCOMMAND);
	for (auto& helpe : Helps)
		InsertMenu(help_menu, -1, MF_BYPOSITION, ID_HELP_BASE + i++, helpe.Title.c_str());
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR command_line, int nCmdShow)
{
	HWND     window;

	app_instance = hInstance;
	string initial_layout_name{};
	string initial_demo_file{};

	auto args = ParseCommandLineToVector(command_line); //handles zero and null case
	for (size_t ano = 0; ano < args.size(); ano++) {
		string arg = args[ano];
		if (arg.length() && (arg[0] == '/' || arg[0] == '-')) {
			string argb = arg.substr(1);
			if (argb == "contacts_per_line") {
				if (ano >= args.size() - 1) {
				badcpl:		 usermsg("Missing or bad number after -contacts_per_line arg.");
					continue;
				}
				if (sscanf(args[++ano].c_str(), "%d", &ContactsPerLine) < 1)
					goto badcpl;
			}
			else if (argb == "automation" || argb == "embedding")
#ifdef NXOLE
				automation = TRUE;
#else
			{
				usermsg("Bad control arg: /%s; this executable of %s is not OLE-enabled.",
					argb.c_str(), PRODUCT_NAME);
				return 0;
			}
#endif
#ifdef NXOLE
			else if (argb == "register") {
				RegisterNXOLE();
				return 0;
			}
			else if (argb == "unregister") {
				UnRegisterNXOLE();
				return 0;
			}
			else if (argb == "script") {
				if (ano >= args.size() - 1) {
					usermsgstop("Missing script file name after %s", arg);
					return 0;
				}
				initial_script_file = args[++ano];
			}
#endif

			else
				usermsg("Bad/unknown control arg: %s", arg.c_str());
		}
		else initial_layout_name = arg;
	}

	if (!hPrevInstance) {
		WNDCLASS klass;
		memset(&klass, 0, sizeof(klass));
		klass.style = CS_BYTEALIGNCLIENT | CS_CLASSDC;
		klass.lpfnWndProc = (WNDPROC)MainWindow_WndProc;
		klass.cbClsExtra = 0;
		klass.cbWndExtra = 10;
		klass.hInstance = hInstance;
		klass.hIcon = LoadIcon(hInstance, "NXICON");
		klass.hCursor = LoadCursor(NULL, IDC_ARROW);
		klass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		klass.lpszMenuName = "NXSYS";
		klass.lpszClassName = MainWindow_Class;

		if (!(RegisterClass(&klass)
			&& RegisterFullSignalWindowClass(hInstance)
			&& RegisterRelayTraceWindowClass(hInstance)
			&& RegisterRelayLogicWindowClass(hInstance)))
			return 0;
	}
	HACCEL hAccel = LoadAccelerators(hInstance, "NXACC");
	window = NULL;   // KRAZY WTF NO WINDOW
	int rv = StartUpNXSYS(hInstance, window,
		NULL0(initial_layout_name), NULL0(initial_demo_file), nCmdShow);

	SetUpLibraryMenu();
	SetUpHelpMenu();

	if (rv == 0)
		rv = WindowsMessageLoop(G_mainwindow, hAccel, 0);

	CleanUpNXSYS();

	return rv;
}

std::filesystem::path GetResourceDirectoryPathname() {
	char buf[512];
	GetModuleFileName(app_instance, buf, sizeof(buf) / sizeof(buf[0]));
	std::filesystem::path p(buf);
	return p.parent_path();
}
