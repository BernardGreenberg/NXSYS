#include <windows.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <commctrl.h>
#include "commands.h"
#include "compat32.h"
#include "nxsysapp.h"
#include "rlytrapi.h"
#include "resource.h"
#pragma comment(lib, "comctl32")
#undef _strdup
static char RTWinClass[] = PRODUCT_NAME ":RelayTrace";

static WNDPROC_DCL RTWin_WndProc (HWND, unsigned, WPARAM, LPARAM);
static void UnMore();

static HWND Window = NULL, TB = NULL;
static HMENU CtlMenu = NULL;
void CheckRelayDisplay();

static TEXTMETRIC TRWin_tm;

static int More = 0, MoreBreak = 0;
static int Lno;
static int NLines = 20;
const int Hlen = 15;
static int Y0;

static char *Data;


struct _ButtonData {
    int Bmx;
    int Style;
    int Command;
    const char * String;
} ButtonData []
	= {
    {0,TBSTYLE_BUTTON, CmTrClear, "Clear the Relay Trace Window"},
    {1,TBSTYLE_CHECK, CmTrMortoggle, "Toggle MORe ON and OFF"},
    {2,TBSTYLE_BUTTON, CmTrMorgo, "Proceed past MORE break"},
    {3,TBSTYLE_BUTTON, CmTrMorstop, "STOP: Abort simulation"},
};

#define BUTTON_DATA_COUNT (sizeof (ButtonData)/sizeof(ButtonData[0]))  

int HandleTrwinToolbarNotification (WPARAM wParam, LPARAM lParam) { 

    if (((LPNMHDR) lParam)->code == TTN_NEEDTEXT) {
	LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam; 
	int idButton = lpttt->hdr.idFrom; 
	if (idButton != 0)
	    for (int i = 0; i < BUTTON_DATA_COUNT;i++)
		if (ButtonData[i].Command == idButton) {
		    lpttt->lpszText = _strdup(ButtonData[i].String);   // WHY DOES THIS WANT NON CONST?
		    return 1;
		}
    } 
    return 0;
}


static void AssertToolbarCheckState (int cmd, int state) {
    if (TB)
	SendMessage (TB, TB_CHECKBUTTON, (WPARAM) cmd,
		     (LPARAM) MAKELONG (state, 0));
}

static void EnableToolButton (int cmd, int val) {
    if (TB)
	SendMessage (TB, TB_ENABLEBUTTON, (WPARAM) cmd,
		     (LPARAM) MAKELONG (val, 0));
}

 void CreateTrwinToolbar (HWND hWnd) {
    InitCommonControls();

    TBBUTTON B [BUTTON_DATA_COUNT];
    memset (B, 0, sizeof(B));

    int bmx = 0;
    for (int i = 0; i < BUTTON_DATA_COUNT; i++) {
	B[i].fsStyle = ButtonData[i].Style;
	if (B[i].fsStyle != TBSTYLE_SEP) {
	    B[i].fsState = TBSTATE_ENABLED;
	    B[i].idCommand = ButtonData[i].Command;
	    B[i].iBitmap = ButtonData[i].Bmx;
	    if (ButtonData[i].Bmx > bmx)
		bmx = ButtonData[i].Bmx;
	}
    }

    TB = CreateToolbarEx
	 ( hWnd,
	   WS_CHILD | TBSTYLE_TOOLTIPS,
	   10901,
	   bmx+1,
	   app_instance,
	   IDB_TRACETOOLS,
	   B,
	   BUTTON_DATA_COUNT,
	   16,16, 16,16,
	   sizeof(TBBUTTON));

    if (TB == NULL)
	FatalAppExit (0, "Can't create trace window toolbar");
    AssertToolbarCheckState (CmTrMortoggle, 0);
    EnableToolButton(CmTrMorgo, 0);
    EnableToolButton(CmTrMorstop, 0);
}


static void RelayTracer (const char * s, int state) {
    char buf[50];
    const char * string;
    if (state == 0)
	string = "DROP";
    else if (state ==1)
	string = "PICK";
    else if (state==-1)
	string = "COMP";
    else string = "????";
    sprintf (buf, "%s %s", string, s);
    int l = strlen (buf);
    if (l > 14) {
	buf[14] = '\0';
	l = 14;
    }
    strcpy (Data + Lno*Hlen, buf);
    if (l >= Hlen)
	l = Hlen;
    if (l < Hlen)
	memset (Data+Lno*Hlen+l, ' ', Hlen - l);

    if (Lno == 0) {
	memset (Data+NLines*Hlen, ' ',Hlen);
	InvalidateRect (Window, NULL, 0);
    }
    else {
	RECT r{};
	r.top = Lno*TRWin_tm.tmHeight + Y0;
	r.bottom = (Lno+2)*TRWin_tm.tmHeight + Y0;
	r.left = 0;
	r.right = TRWin_tm.tmAveCharWidth * Hlen;
	InvalidateRect (Window, &r, 0);
    }
    memset (Data+(Lno+1)*Hlen, '*', Hlen);
    if (++Lno >= NLines) {
	if (More) {
	    CheckRelayDisplay();
	    EnableToolButton(CmTrMorgo, 1);
	    EnableToolButton(CmTrMorstop, 1);
	    memmove(Data+Lno*Hlen, "**MORE**  ", 10);
	    UpdateWindow (Window);
	    MoreBreak = 1;
	    SetFocus(Window);
	    WindowsMessageLoop (Window, NULL, WM_NXSYS_CLOSE_TRACE_WINDOW);
	    SetFocus (G_mainwindow);
	    MoreBreak = 0;
	}
	Lno = 0;
    }
    UpdateWindow (Window);
}

static void CreateTraceWindow () {

	RECT rc{};
    SystemParametersInfo (SPI_GETWORKAREA, 0, &rc, 0); /* 23 Dec 2000*/
    int dtw = rc.right-rc.left;
    int dth = rc.bottom-rc.top - GetSystemMetrics (SM_CYHSCROLL);
    int rww = dtw/8;
    Window = CreateWindow (RTWinClass, "Relay Trace",
			   WS_OVERLAPPED |WS_SYSMENU|
			   WS_CAPTION | WS_BORDER,
			   dtw - rww,
			   0,
			   rww,
			   dth,
			   G_mainwindow,
			   NULL,		/* menu */
			   app_instance,
			   NULL);		/* MDI data */

    if (Window == NULL)
	FatalAppExit (0, "Can't create Relay Trace Window.");
    CreateTrwinToolbar(Window);
    GetWindowRect (TB, &rc);
    Y0 = rc.bottom - rc.top;
    CtlMenu = GetSubMenu (GetMenu(Window), 0);
    SendMessage (Window, WM_SETFONT, (WPARAM) GetStockObject(ANSI_FIXED_FONT), 0L);
    HDC dc = GetDC(Window);
    SelectObject (dc,  GetStockObject(ANSI_FIXED_FONT));
    GetTextMetrics (dc, &TRWin_tm);
    ReleaseDC(Window, dc);
    GetClientRect (Window, &rc);
    rc.top = Y0;
    NLines = (rc.bottom-rc.top)/(TRWin_tm.tmHeight) - 1;
    rww = (Hlen+1)*TRWin_tm.tmAveCharWidth;
    MoveWindow (Window, dtw-rww, 0, rww,
		(NLines+1) * TRWin_tm.tmHeight + Y0 +
		2*GetSystemMetrics(SM_CYBORDER) + 
		GetSystemMetrics (SM_CYMENU) +
		GetSystemMetrics (SM_CYCAPTION), 0);
    Data = new char [(NLines+1)*Hlen];
    ShowWindow (TB, SW_SHOWNOACTIVATE);
}

static void PostRTWClose () {
    PostMessage (Window, WM_NXSYS_CLOSE_TRACE_WINDOW, 0, 0);
}

BOOL RelayTraceExposedP () {
    return Window && (GetWindowLong(Window, GWL_STYLE) & WS_VISIBLE);
}

void EnableRelayTrace (BOOL enable_it) {

    static int made = 0;
    if (!enable_it) {
	if (made && RelayTraceExposedP()) {
	    UnMore();
	    PostRTWClose();
	    RTWin_WndProc (Window, WM_COMMAND, CmTrHide, 0);
	}
	return;
    }

    if (!made) {
	CreateTraceWindow();
	made = 1;
    }

    MoreBreak = 0;
    SetRelayTrace (RelayTracer); /* could be new reload */
    for (int i = 0; i <= NLines; i++)
	memset (Data+i*Hlen, ' ', Hlen);
    Lno = 0;
    ShowWindow (Window, SW_SHOWNOACTIVATE);
}


static void UnMore () {
    if (MoreBreak) {
	MoreBreak = 0;
	EnableToolButton(CmTrMorgo, 0);
	EnableToolButton(CmTrMorstop, 0);
	Lno = 0;
    }
}




static void MaybeAbort (HWND hwnd) {
    if (IDYES ==
	MessageBox (hwnd,
		    "Really abort this scenario?",
		    PRODUCT_NAME " Relay Trace",
		    MB_ICONEXCLAMATION | MB_YESNOCANCEL)) {

	UnMore();
	SetFocus (G_mainwindow);
	NxsysAppAbort (0, "\"Q\" typed to Relay Trace Window");
    }
    else 
	UnMore();

}

static WNDPROC_DCL RTWin_WndProc
   (HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
  int i;
  switch (message) {

    case WM_NOTIFY:
        return HandleTrwinToolbarNotification(wParam, lParam);
		  
    case WM_COMMAND:
	switch (wParam) {
	    case CmTrHide:
		ShowWindow (window, SW_HIDE);
		UnMore();
		SetRelayTrace (NULL); //fall through
	    case CmTrClear:
		for (i = 0; i < NLines; i++)
		    memset (Data+i*Hlen, ' ', Hlen);
		Lno = 0;
		InvalidateRect (window, NULL, 0);
		if (MoreBreak)
		    goto morgo;
		break;
	    case CmTrMortoggle:
		return RTWin_WndProc (window, WM_COMMAND,
				      More ? CmTrMoroff : CmTrMoron, 0);
	    case CmTrMoron:
		More = 1;
		CheckMenuItem (CtlMenu, CmTrMortoggle, MF_BYCOMMAND | MF_CHECKED);
		AssertToolbarCheckState (CmTrMortoggle, 1);
		break;
	    case CmTrMoroff:
		More = 0;
		CheckMenuItem
			(CtlMenu, CmTrMortoggle, MF_BYCOMMAND | MF_UNCHECKED);
		AssertToolbarCheckState (CmTrMortoggle, 0);
		if (!MoreBreak)
		    break;
		/* fall through */
	    case CmTrMorgo:
morgo:
		UnMore();
		PostRTWClose();;
		break;
	    case CmTrMorstop:
		MaybeAbort(window);
		PostRTWClose();;
		break;
	    default:;
	}
	return 0L;

    case WM_PAINT:
    {
	PAINTSTRUCT ps;
	HDC dc = BeginPaint (window, &ps);
	SelectObject (dc, GetStockObject(ANSI_FIXED_FONT));
	for (i = 0; i <= NLines; i++) {
		RECT r{};
	    r.left = 0;
	    r.top = i*TRWin_tm.tmHeight + Y0;
	    r.bottom = r.top+NLines * TRWin_tm.tmHeight + Y0;
	    r.right = r.left + Hlen*TRWin_tm.tmAveCharWidth;
	    DrawText (dc, Data+i*Hlen, Hlen, &r, DT_LEFT|DT_TOP|DT_NOCLIP);
	}
	EndPaint (window, &ps);
	break;
    }

    case WM_CLOSE:
    {
	SetRelayTrace (NULL);
	ShowWindow (window, SW_HIDE);
	for (i = 0; i < NLines; i++)
	    memset (Data+i*Hlen, ' ', Hlen);
	Lno = 0;
	if (MoreBreak)
	    PostRTWClose();;
	UnMore();
	break;
    }
    case WM_CHAR:
	if (MoreBreak) {
	    if (LOWORD(wParam) == 'q' || LOWORD(wParam) == 'Q')
		MaybeAbort(window);
	    else
		UnMore();
	    PostRTWClose();;
	}
	return 0;
   default:
        return DefWindowProc (window, message, wParam, lParam);
  }
  return 0;
       
}

BOOL RegisterRelayTraceWindowClass (HINSTANCE hInstance) {
    WNDCLASS klass;
    memset (&klass, 0, sizeof(klass));

    klass.style          = CS_BYTEALIGNCLIENT | CS_CLASSDC;
    klass.lpfnWndProc    = (WNDPROC) RTWin_WndProc;
    klass.cbClsExtra     = 0;
    klass.cbWndExtra     = 10;
    klass.hInstance      = hInstance;
    klass.hIcon          = LoadIcon (hInstance, "RELAYICON");
    klass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    klass.hbrBackground  = (HBRUSH)GetStockObject (WHITE_BRUSH);
    klass.lpszMenuName   = "RLYTRMEN";
    klass.lpszClassName  = RTWinClass;

    return RegisterClass (&klass);
}
