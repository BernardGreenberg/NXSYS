/* split off from signal.cpp 18 February 1997 */

#include "windows.h"
#include <string.h>
#include <stdio.h>

#include "lyglobal.h"
#include "signal.h"
#include "compat32.h"
#include "nxsysapp.h"
#include "brushpen.h"
#include "fsigctl.h"

#include <iterator>
#include "STLExtensions.h"

#if NXSYSMac
HWND MacMakeFSW(int x, int y, int w, int h, void*signal);
void MacFillPlateSexily(HWND, int, int, int, int);
#endif

#include "xtgtrack.h"

#if WIN32
static char SigWinClass[] = PRODUCT_NAME ":SignalAspect";
#endif

SigHead::SigHead(const std::string& lights, const std::string& plate) :
   Lights(lights), Plate(plate), State('X')
{
    height = (short)lights.size();
}


const int SigWinTopDisp = 10,
SigWinTopToFirstCenter = 15,
SigWinInterCenter = 25,
SigWinRadius = 10,
SigWinStrokeWidth = 1,
SigWinInterHead = 5,
SigWinPipeWidth = 10,
SigWinPipeExtra = 20,
SigWinHeadWidth = 30,
SigWinWidth = 60;

const int SigWinStopHeight = 16,
SigWinStopWidth = 15;



static HFONT Font = NULL, SFont = NULL;

#if WIN32
HWND SigWins[100];
int  SigWinCount = 0;
static int LastX = 0;
#endif

void DestroySigWins() {
#if WIN32
	LastX = 0;
	for (int i = 0; i < SigWinCount; i++) {
		HWND win = SigWins[i];
		((Signal*)GetWindowLongPtr(win, 0))->Window = NULL;
		DestroyWindow(win);
	}
	if (Font != NULL)
		DeleteObject(Font);
	if (SFont != NULL)
		DeleteObject(SFont);
	Font = SFont = NULL;
	SigWinCount = 0;
#endif
}


HWND MakeSigWin(Signal * s, int x, int y) {
    if (Font == NULL) {
        LOGFONT lf;
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = SigWinHeadWidth / 2;
        strcpy(lf.lfFaceName, "Helvetica");
        lf.lfWeight = FW_BOLD;
        Font = CreateFontIndirect(&lf);
        memset(&lf, 0, sizeof(LOGFONT));
        lf.lfHeight = (int)(SigWinHeadWidth / 1.3);
        strcpy(lf.lfFaceName, "Helvetica");
        lf.lfWeight = FW_BOLD;
        SFont = CreateFontIndirect(&lf);
    }

#if NXSYSMac
    // This is designed clearance to top of object (Panel or View), whatever, not title-height.
    int height_overhead = 20;
#else
    int height_overhead = 2 * GetSystemMetrics(SM_CYBORDER) +  // Appelez "Fenêtres sans Frontiers"
    GetSystemMetrics(SM_CYCAPTION) + 20;
    // 11-28-2016 -- Windows FSW's seem exactly that much (20) short. Ich weiss nicht warum.
#endif

    int winheight = SigWinTopDisp + s->UnitHeight() + height_overhead;

    RECT rc;
    GetWindowRect(GetDesktopWindow(), &rc);
#if BOTTOM_POS_SIG_WINS
    int dtw = rc.right - rc.left;
    int dth = rc.bottom - rc.top;
    int sigwinx = LastX,
    int sigwiny = dth - winheight,

    LastX += SigWinWidth;
    if (LastX + SigWinWidth > dtw)
        LastX = 0;
#else
    int sigwinx = x;
    int sigwiny = y;
#endif

#if NXSYSMac
    HWND window = MacMakeFSW(sigwinx, sigwiny, SigWinWidth, winheight, s);
#else
    HWND window = CreateWindow(SigWinClass, "Signal",
                               WS_OVERLAPPED | WS_SYSMENU |
                               WS_CAPTION | WS_BORDER,
                               sigwinx, sigwiny,
                               SigWinWidth, winheight,
                               G_mainwindow, NULL, app_instance, NULL);

    SendMessage(window, WM_SETFONT, (WPARAM)Font, 0);
    SetWindowLongPtr(window, GWLP_USERDATA, (INT_PTR)s);
#endif
    char plate[20];
    if (s->XlkgNo)
        if (s->XlkgNo >= 7000)
            sprintf(plate,
                    "X %c%d",
                    'A' + ((s->XlkgNo / 1000) - 7),
                    s->XlkgNo % 1000);
        else
            sprintf(plate, "X %d", s->XlkgNo);
    else
        strcpy(plate, s->CompactName().c_str());
    SetWindowText(window, plate);
#if WIN32
    SigWins[SigWinCount++] = window;
#endif
    return window;
}

int Signal::UnitHeight() {
    int u = 0;
    for (auto& h : Heads) {
        u += h.UnitHeight();
        u += SigWinInterHead;
    }
    u += SigWinPipeExtra / 2;
    return u;
}


char Signal::ComputeState() {
    int clear = HG;			/* might be HV, you know... */
    Heads[0].State = clear ? (DG ? 'G' : 'Y') : 'R';
    for (auto& h : Ranger<decltype(Heads)>(Heads, 1)) {
        if (h.height > 1)
            h.State = clear ? (DivG ? 'Y' : 'G') : 'R';
        else {
            int lit;
            switch (h.Lights[0]) {
                case 'S':
                    lit = (MiscG & SIGF_S) && clear;
                    break;
                case 'D':
                    lit = (MiscG & SIGF_D) && clear;
                    break;
                case 'Y':
                    lit = (MiscG & SIGF_CO);
                    break;
                case 'W':
                    lit = (MiscG & SIGF_LUNAR)
                    || ((MiscG & SIGF_LUNAR_WHEN_RED) && !clear);
                    break;
                default:
                    lit = 0;
            }
            h.State = lit ? h.Lights[0] : 'X';
        }
    }
    return Heads[0].State;
}


static void DrawPipe(HDC dc, int x, int y, int length) {
    RECT r{};
    r.left = x - SigWinPipeWidth / 2;
    r.right = x + SigWinPipeWidth / 2;
    r.top = y + 1;
    r.bottom = y + length;
    FillRect(dc, &r, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    MoveTo(dc, r.left, r.top);
    LineTo(dc, r.left, r.bottom);
    MoveTo(dc, r.right, r.top);
    LineTo(dc, r.right, r.bottom);
}


void Signal::WinDisp(HDC dc, int lights_only) {
	/* this won't win the ACM Turing Award, but it will prevent some crashes until this is fixed correctly. */
    if (Window == NULL)
        return;

    ComputeState();
    int y = SigWinTopDisp;
    int xc = SigWinWidth / 2;
    int stp = (MiscG & SIGF_STR) && Heads[0].State == 'R';
    int i = 0;
    for (auto& h : Heads) {
        y = h.DisplayWin(this, xc, y, dc, lights_only, stp);
        int pipel = SigWinInterHead;
        if (i == Heads.size() - 1)
            pipel += SigWinPipeExtra;
        if (!lights_only)
            DrawPipe(dc, xc, y, pipel);
        y += pipel;
        i += 1;
    }
    if (!lights_only)
        DisplayStop(dc);
}

#if NXSYSMac
void CallFSWinDisp(void* s, HDC dc, int lo) {
	((Signal*)s)->WinDisp(dc, lo);
}

#endif

void Signal::UpdateStop() {
    if (Window) {
#if NXSYSMac
        Invalidate();
#else
        HDC dc = GetDC(Window);
        DisplayStop(dc);
        ReleaseDC(Window, dc);
#endif
    }
}

void Signal::DisplayStop(HDC dc) {

    RECT cli{}, sb{};

    if (TStop == NULL)
        return;

    GetClientRect(Window, &cli);

    sb.top = cli.bottom - SigWinStopHeight;
    sb.bottom = cli.bottom;
    sb.left = 0;// For right-side, was SigWinWidth/2 + (SigWinHeadWidth*3)/5;
    sb.right = sb.left + SigWinStopWidth;

    TStop->SigWinDisplay(dc, &sb);
}


int SigHead::UnitHeight() {
	return 2 * SigWinTopToFirstCenter + (height - 1)*SigWinInterCenter
		+ (Plate[0] ? SigWinHeadWidth : 0);
}

static void DrawAlphanumericAspect(HDC dc, const char* sd, int sdlen, int x, int y)
{
    RECT r{};
    SelectObject(dc, SFont);
    r.left = r.top = 0;
    DrawText(dc, sd, sdlen, &r, DT_NOCLIP | DT_CENTER | DT_TOP | DT_CALCRECT);
    int w = r.right / 2;
    int h = r.bottom / 2;
    r.left = x - w; r.right = x + w;
    r.top = y - h; r.bottom = y + h;
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkMode(dc, TRANSPARENT);
    DrawText(dc, sd, sdlen, &r, DT_NOCLIP | DT_CENTER | DT_TOP);
    SetTextColor(dc, RGB(0, 0, 0));
    SetBkMode(dc, OPAQUE);
    SelectObject(dc, Font);
    SetBkMode(dc, TRANSPARENT);
}

static void DrawLens(HDC dc, int x, int y, char c) {
    
    HBRUSH brush;
    
    switch (c) {
        case 'X':  brush = GKOffBrush;      break;
        case 'Y':  brush = GKYellowBrush;   break;
        case 'R':  brush = GKRedBrush;      break;
        case 'G':  brush = GKGreenBrush;    break;
        case 'W':  brush = GKWhiteBrush;    break;
        case 'S':
        case 'D':  brush = GKOffBrush;      break;
        default:   brush = GKOffBrush;      break;
    }
    
    SelectObject(dc, brush);
    Ellipse(dc, x - SigWinRadius, y - SigWinRadius,
            x + SigWinRadius, y + SigWinRadius);
    
    if (c == 'S' || c == 'D')
        DrawAlphanumericAspect(dc, &c, 1, x, y);
}


static void DrawSTBox(HDC dc, int x, int y) {
    RECT r{};
    r.left = x - (SigWinHeadWidth / 2);
    r.right = x + (SigWinHeadWidth / 2);
    r.top = y - SigWinInterCenter / 2;
    r.bottom = r.top + SigWinInterCenter;
    FillRect(dc, &r, GKOffBrush);
}


static void DrawPlate(Signal* S, HDC dc, int x, int y, int height, const char * plate) {
	int hw = SigWinHeadWidth / 2;
	int yb = y + height;
        RECT r{};
	MoveTo(dc, x - hw, y);
	LineTo(dc, x - hw, yb);
	LineTo(dc, x + hw, yb);
	LineTo(dc, x + hw, y);

	r.left = x - hw;
	r.top = y + SigWinStrokeWidth;
	r.bottom = yb;
	r.right = x + hw;
	SetBkColor(dc, RGB(255, 255, 255));
	SelectObject(dc, Font);
#if NXSYSMac
	MacFillPlateSexily(S->Window, r.left, r.top, r.bottom - r.top, r.right - r.left);
	const char* lnp = strchr(plate, '\n');
	if (lnp) {
            size_t ln1 = lnp - plate;
            size_t pos2 = lnp - plate + 1;
            size_t ln2 = strlen(plate) - pos2;
            RECT r1 = r;
            int toth = r.bottom - r.top;
            r1.bottom = r1.top + toth / 2;
            DrawText(dc, plate, ln1, &r1, DT_TOP | DT_CENTER);
            r1.top = r1.bottom;
            r1.bottom = r.bottom;
            DrawText(dc, plate + pos2, ln2, &r1, DT_TOP | DT_CENTER);
        }
	else {
		DrawText(dc, plate, strlen(plate), &r, DT_TOP | DT_CENTER);
	}
#else
	DrawText(dc, plate, (int)strlen(plate), &r, DT_TOP | DT_CENTER);
#endif
	if (Glb.IRTStyle) {
            int xw = (int)(.75f*hw);
            int yl = y + height / 2;
            MoveTo(dc, x - xw, yl);
            LineTo(dc, x + xw, yl);
	}
}

int SigHead::DisplayWin(Signal* S,
    int xc, int headtop, HDC dc, int lights_only,
    int stp) {
    int y = headtop + SigWinTopToFirstCenter;
    int hw = SigWinHeadWidth / 2;

    if (!lights_only) {
        RECT hr{};
        hr.left = xc - hw;
        hr.right = xc + hw;
        hr.top = headtop;
        hr.bottom = hr.top + (height - 1)*SigWinInterCenter
        + 2 * SigWinTopToFirstCenter;
        FillRect(dc, &hr, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    }

    for (int l = 0; l < height; l++) {
        char c = Lights[l];
        if (c == 'T') {
            DrawSTBox(dc, xc, y);
            if (stp)
                DrawAlphanumericAspect(dc, "20", 2, xc, y);
        }
        else if (c == 'Z');
        else if (c == State)
            DrawLens(dc, xc, y, c);
        else
            DrawLens(dc, xc, y, 'X');
        y += SigWinInterCenter;
    }
    y += SigWinTopToFirstCenter - SigWinInterCenter;

    if (!lights_only) {
        MoveTo(dc, xc - hw, headtop);
        LineTo(dc, xc - hw, y);
        LineTo(dc, xc + hw, y);
        LineTo(dc, xc + hw, headtop);
        LineTo(dc, xc - hw, headtop);
    }
    
    if (Plate[0]) {
        if (!lights_only)
            DrawPlate(S, dc, xc, y, 2 * hw, Plate.c_str());
        y += 2 * hw;
    }

    return y;
}

#if WIN32
static WNDPROC_DCL SigWin_WndProc
(HWND window, unsigned message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
        case WM_PAINT:
	{
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(window, &ps);
            SelectObject(dc, Font);
            ((Signal*)GetWindowLongPtr(window, GWLP_USERDATA))->WinDisp(dc, 0);
            EndPaint(window, &ps);
            break;
	}
	case WM_CLOSE:
            ShowWindow(window, SW_HIDE);
            break;
	default:
            return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
}

BOOL RegisterFullSignalWindowClass(HINSTANCE hInstance) {

	WNDCLASS klass;
	memset(&klass, 0, sizeof(klass));
	klass.style = CS_BYTEALIGNCLIENT | CS_CLASSDC;
	klass.lpfnWndProc = (WNDPROC)SigWin_WndProc;
	klass.cbClsExtra = 0;
	klass.cbWndExtra = 10;
	klass.hInstance = hInstance;
	klass.hIcon = LoadIcon(hInstance, "NXICON");
	klass.hCursor = LoadCursor(NULL, IDC_ARROW);
	klass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	klass.lpszMenuName = NULL;
	klass.lpszClassName = SigWinClass;

	return RegisterClass(&klass);
}
#endif
