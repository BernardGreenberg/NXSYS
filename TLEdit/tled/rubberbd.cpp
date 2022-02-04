#include "windows.h"
#include "rubberbd.h"

static HPEN Pen = NULL;
static HPEN HighlightPen = NULL;

#ifdef NXSYSMac
#error Rubberbd.cpp is not used on the Mac.  Get your act together.
#endif

void RubberBandInit() {
    Pen = CreatePen (PS_SOLID, 0, RGB(0, 255, 255));
    HighlightPen = CreatePen (PS_SOLID, 0, RGB(255, 0, 0));
}

void RubberBandClose() {
    if (Pen)
	DeleteObject(Pen);
    if (HighlightPen)
	DeleteObject(HighlightPen);
    HighlightPen = NULL;

}

RubberBand::RubberBand (HWND h_Wnd, int px, int py, int j) {
    hWnd = h_Wnd;
    J = j;
    hDC = GetDC(h_Wnd);
    StartX = px;
    StartY = py;
    Drawn = FALSE;
}

void RubberBand::Undraw() {
    if (Drawn) {
	SetROP2(hDC, R2_XORPEN);
	SelectObject (hDC, LastPen);
	MoveToEx (hDC, StartX, StartY, NULL);
	LineTo (hDC, EndX, EndY);
	SetROP2(hDC, R2_COPYPEN);
    }
    Drawn = FALSE;
}

void RubberBand::Draw (int x, int y) {
    if (Drawn)
	Undraw();
    SelectObject (hDC,LastPen = Pen);
    SetROP2(hDC, R2_XORPEN);
    MoveToEx (hDC, StartX, StartY, NULL);
    LineTo (hDC, EndX = x, EndY = y);
    SetROP2(hDC, R2_COPYPEN);
    Drawn = TRUE;
}

void RubberBand::DrawHighlighted (int x, int y) {
    if (Drawn)
	Undraw();
    SelectObject (hDC,LastPen = HighlightPen);
    SetROP2(hDC, R2_XORPEN);
    MoveToEx (hDC, StartX, StartY, NULL);
    LineTo (hDC, EndX = x, EndY = y);
    SetROP2(hDC, R2_COPYPEN);
    Drawn = TRUE;
}


RubberBand::~RubberBand() {
    Undraw();
    ReleaseDC (hWnd, hDC);
}
