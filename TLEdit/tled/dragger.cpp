#include "windows.h"

#include <stdio.h>
#include <math.h>
#include <cassert>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "dragger.h"
#include "resource.h"
#include "undo.h"

Dragger * RodentatingDragon = NULL;
Dragger * MouseupDragon = NULL;
#ifdef NXSYSMac
void MacDragonOn(), MacDragonOff();
#endif


Dragger::Dragger () {
    Object = NULL;
    MovingState = MoveStates::NOT;
}

void Dragger::TruncMouse (HWND hWnd, int x, int y, WP_cord & wpx, WP_cord & wpy) {
    RECT r;
    GetClientRect (hWnd, &r);
    if (x < r.left) x = r.left;
    if (y < r.top) y = r.top;
    if (x > r.right) x = r.right;
    if (y > r.bottom) y = r.bottom;
    wpx = SCXtoWP (x) + Xoff;
    wpy = SCYtoWP (y) + Yoff;
}


GraphicObject* Dragger::StartMoving (GraphicObject * g, const char * description, HWND hWnd) {
    Description = description;
    MovingState = MoveStates::FROM_MOUSEUP;
    MouseupDragon = RodentatingDragon = this;
    Xoff = 0;
    Yoff = 0;
    Object = g;
    Object->MakeSelfVisible();
    StatusMessage ("Move mouse to new position and click left to drop, right to abort.");
#ifdef NXSYSMac
    MacDragonOn();
#endif
    return g;
}

void Dragger::Rodentate (HWND hWnd, int x, int y, UINT message, WPARAM wParam) {

    WP_cord wpx, wpy;
    const char * remark = "  ";

    UINT endbutton = 0;
    switch (MovingState) {
        case MoveStates::NOT:
	    return;
        case MoveStates::FROM_MOUSEUP:
	    endbutton = WM_LBUTTONDOWN;
	    if (message == WM_RBUTTONDOWN) {
		Abort();
		return;
	    }
	    remark = "Click left to drop, right to abort";
	    break;
        case MoveStates::FROM_MOUSEDOWN:
	    endbutton = WM_LBUTTONUP;
	    remark ="Release mouse button to drop in place";
	    break;
    }

    TruncMouse (hWnd, x, y, wpx, wpy);

    Object->Hide();
    Object->wp_x = wpx;		/* default move method too slow */
    Object->wp_y = wpy;
    Object->ComputeWPRect();
    Object->ComputeVisibleLast();
    Object->UnHide();

    switch (message) {
	case WM_LBUTTONUP:
	    remark = "Click right on it to edit lever #, drag left to move";
	    break;
	case WM_LBUTTONDOWN:
            Undo::RecordGOMoveStart(Object);
	    remark = "Drag to move, release mouse to drop.";
	    break;
    }

    StatusMessage ("%s at (%d, %d) %s", Description.c_str(), wpx, wpy, remark);
    if (message == endbutton) {
        if (MovingState == MoveStates::FROM_MOUSEUP) // Creation
            Undo::RecordGOCreation(Object);
        else if (MovingState == MoveStates::FROM_MOUSEDOWN)  //Movation
            Undo::RecordGOMoveComplete(Object);
        else {
            assert (!"Dragon unclear");
        }
            
	MovingState = MoveStates::NOT;
	MouseupDragon = RodentatingDragon = NULL;
	Object->Select();
	Object = NULL;
#ifdef NXSYSMac
        MacDragonOff();
#endif
    }
}

void Dragger::ClickOnWNum(HWND hWnd, GraphicObject * g,
                          const char * description,
                          long num, int x, int y) {
    std::string text = description;
    text += " " + std::to_string(num);
    ClickOn(hWnd, g, text.c_str(), x, y);
}

void Dragger::ClickOn (HWND hWnd, GraphicObject * g,
                       const char * description, int x, int y) {
    if (Movingp())
	return;
    if (!g->Selected) {
	g->Select();
	StatusMessage ("%s at (%d, %d) - click again and drag to move",
		       description,
		       g->wp_x, g->wp_y);
	return;
    }
    Description = description;
    MovingState = MoveStates::FROM_MOUSEDOWN;
    RodentatingDragon = this;
    Object = g;
    Xoff = 0;
    Yoff = 0;
    WP_cord wpx, wpy;
    TruncMouse (hWnd, x, y, wpx, wpy);
    Xoff = (int)(g->wp_x - wpx);
    Yoff = (int)(g->wp_y - wpy);
#ifdef NXSYSMac
    MacDragonOn();
#endif
    Rodentate (hWnd, x, y, WM_LBUTTONDOWN, 0);
}

void Dragger::Abort () {
    if (MovingState != MoveStates::FROM_MOUSEUP)
	return;
    delete Object;			/* needs virtual or callback */
    MovingState = MoveStates::NOT;
    StatusMessage ("  ");
#ifdef NXSYSMac
    MacDragonOff();
#endif
}

/* virtual */
void Dragger::DeleteObj() {
    delete Object;
}
