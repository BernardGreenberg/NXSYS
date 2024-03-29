#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include "xtgtrack.h"
#include "xturnout.h"
#include "swkey.h"
#include "typeid.h"
#include "ssdlg.h"
#include "brushpen.h"
#include "compat32.h"
#include "rlyapi.h"

#include "nxsysapp.h"

/* In one evening Feb 1 1997 */

#define LIGHT_LEGEND_DT_OPTS \
	DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP
#define NUM_DT_OPTS DT_CENTER |DT_SINGLELINE |DT_NOCLIP

static int Margin = 3, BottomMargin= 2;
static int Top, Bottom, Width, Height, NumHeight;
static int LKRadius,LKOffset;
#if 0
static Turnout ** AllSwitches = NULL;
static SwitchKey ** Keys = NULL;
static int XFill;
static int ctSwitches;
#endif
#ifndef Virtual
#define Virtual
#endif

#ifndef TLEDIT
#include "resource.h"
#include "rlymenu.h"
#endif

#if 0

static int SwitchByXlkgNoComparer (const void * v1, const void * v2) {
    Turnout * t1 = *((Turnout **)v1);
    Turnout * t2 = *((Turnout **)v2);
    if (t1->XlkgNo < t2->XlkgNo)
	return -1;
    if (t1->XlkgNo > t2->XlkgNo)
	return +1;
    return 0;/* shouldn't be ==!, but don't hang here for tledit screwup */
}
#endif

#ifndef TLEDIT
void SwitchKey::LSReporter (BOOL state, void * v) {
    ((SwitchKey *)v)->LSReport(state);
}

void SwitchKey::LSReport (BOOL state) {
    LSState = state;
    Turn->LSReporter (state, Turn);
    Invalidate();
}
#endif

#ifdef TLEDIT
SwitchKey::SwitchKey (int xno, WP_cord p_wpx, WP_cord p_wpy) :
   Reverse(FALSE), Normal(FALSE)
#else
SwitchKey::SwitchKey (Turnout * t, WP_cord p_wpx, WP_cord p_wpy) :
   Turn(t), Reverse(FALSE), Normal(FALSE)
#endif
{
    wp_x = p_wpx;
    wp_y = p_wpy;
    wp_limits.left   = -Margin;
    wp_limits.right  = Width + Margin;
    wp_limits.top    = -(LKRadius + LKOffset);
    wp_limits.bottom = Height + NumHeight + BottomMargin;

    LSState = TRUE;

#ifndef TLEDIT
    if (Turn)
	SetXlkgNo (Turn->XlkgNo);
    else
	SetXlkgNo (0);
    AssociateTurnout (Turn);
#else
    SetXlkgNo (xno);
#endif
}

#ifndef TLEDIT
void SwitchKey::AssociateTurnout (Turnout * t) {
    if (t) {
        ReporterRelay = SetReporterIfExists (t->XlkgNo, "LK", LSReporter, this);
        if (!ReporterRelay)
            ReporterRelay = SetReporterIfExists (t->XlkgNo, "LS", LSReporter, this);
        // Might still be null.
	SetXlkgNo (t->XlkgNo);
    }
    Turn = t;
}
#endif

void SwitchKey::SetXlkgNo (int xno) {
    XlkgNo = xno;
    snprintf (NumStr, sizeof(NumStr), "%d", xno);
    NumStrLen = (int)strlen(NumStr);
}


void InitSwitchKeyData () {
    int gu = GU2;			/* expected to be about 3 */ 
    Width = 6*gu;
    Height = Width*2;
    LKRadius = (int) (1.7*gu);
    LKOffset = (int) (1.5*LKRadius);

    Top = 0;
    Bottom = Top + Height;

    HDC hDC = GetDC (G_mainwindow);
    SelectObject (hDC, Fnt);
    RECT r;
    r.top = r.left = 0;
    DrawText (hDC, "1", 1, &r, NUM_DT_OPTS | DT_CALCRECT);
    NumHeight = r.bottom;
    Bottom += r.bottom + BottomMargin;
    ReleaseDC (G_mainwindow, hDC);
}


Virtual void SwitchKey::Display (HDC hdc) {

    int width = Width;
    int height = Height;
    if (NXGO_Scale < 1.0) {
	width = (int) (width*NXGO_Scale);
	height = (int) (height*NXGO_Scale);
    }
    RECT r, rr, rn;
    r.left = sc_x;
    r.right = sc_x + width;
    r.top = sc_y;
    r.bottom = sc_y + height;
    rn = rr = r;
    rn.bottom = rr.top = sc_y + height/2;

    HBRUSH dftbrush = GKOffBrush ;
#ifdef TLEDIT
    if (Selected)
	dftbrush = GKGreenBrush;
#endif
    SelectObject (hdc, GetStockObject (NULL_PEN));
    if (!Normal && !Reverse) {
	SelectObject (hdc, dftbrush);
	Rectangle (hdc, r.left, r.top, r.right, r.bottom);
    }
    else {
	SelectObject (hdc, Normal ? GKGreenBrush : dftbrush);
	Rectangle (hdc, rn.left, rn.top, rn.right, rn.bottom);
	SelectObject (hdc, Reverse ? GKYellowBrush : dftbrush);
	Rectangle (hdc, rr.left, rr.top, rr.right, rr.bottom);
    }
    COLORREF tcold = GetTextColor (hdc);
    int bm = GetBkMode (hdc);
    SetBkMode (hdc, TRANSPARENT);
    SetTextColor (hdc, RGB (0, 0, 0));
    DrawText (hdc, "N", 1, &rn, LIGHT_LEGEND_DT_OPTS);
    DrawText (hdc, "R", 1, &rr, LIGHT_LEGEND_DT_OPTS);
    SetTextColor (hdc, tcold);
    SetBkMode (hdc, bm);
    SelectObject (hdc, GetStockObject (BLACK_PEN));
    MoveTo (hdc, rn.left, rn.top);
    LineTo (hdc, rn.right, rn.top);
    SelectObject (hdc, GKOffBrush);

    RECT tr = r;
    tr.top = r.bottom + BottomMargin;
    tr.bottom = tr.top + NumHeight;
    DrawText (hdc, NumStr, NumStrLen, &tr, NUM_DT_OPTS);

    /* Lock light */
    SelectObject (hdc, LSState ? GKOffBrush : GKRedBrush);
    int rad = (int)(LKRadius * NXGO_Scale + .8);
    int xcen = sc_x + width/2;
    int ycen = sc_y - (int)(LKOffset*NXGO_Scale + .8);
    Ellipse(hdc, xcen - rad, ycen - rad, xcen + rad, ycen + rad);
}

Virtual TypeId SwitchKey::TypeID() {
    return TypeId::SWITCHKEY;
}

Virtual bool SwitchKey::IsNomenclature(long id) {
    return XlkgNo == id;
}

#ifndef TLEDIT
Virtual void SwitchKey::EditContextMenu (HMENU m) {
    BOOL nhold = RelayState(Turn->NL);
    BOOL rhold = RelayState(Turn->RL);
    if (!Turn->Thrown || Normal || nhold)
	EnableMenuItem(m, ID_NORMAL, MF_BYCOMMAND|MF_GRAYED);
    if (Turn->Thrown || Reverse || rhold)
	EnableMenuItem(m, ID_REVERSE, MF_BYCOMMAND|MF_GRAYED);
    if (nhold)
	EnableMenuItem(m, ID_NORMAL_HOLD, MF_BYCOMMAND|MF_GRAYED);
    if (rhold)
	EnableMenuItem(m, ID_REVERSE_HOLD, MF_BYCOMMAND|MF_GRAYED);
    if (!(rhold || nhold))
	EnableMenuItem(m, ID_UNHOLD, MF_BYCOMMAND|MF_GRAYED);
}


Virtual void SwitchKey::Hit (int mb) {

    int height = Height;
    if (NXGO_Scale < 1.0)
	height = (int) (height*NXGO_Scale);

    if (NXGOHitY > sc_y + height)
	return;

    if (mb == WM_NXGO_RBUTTONCONTROL) {
	switch (RunContextMenu (IDR_SWITCH_KEY_CONTEXT_MENU)) {

	    case ID_NORMAL:
		Press (FALSE, FALSE);
		break;
	    case ID_REVERSE:
		Press (TRUE, FALSE);
		break;
	    case ID_NORMAL_HOLD:
		Press (FALSE, TRUE);
		break;
	    case ID_REVERSE_HOLD:
		Press (TRUE, TRUE);
		break;
	    case ID_UNHOLD:
		ClearAux();
		break;
	    case ID_DRAW_RELAY:
		DrawRelaysForObject (Turn->XlkgNo, "Switch");
		break;
	    case ID_RELAY_QUERY:
		ShowStateRelaysForObject (Turn->XlkgNo, "Switch");
		break;
	}
	return;
    }

    HitWasControl = (mb == WM_RBUTTONDOWN);

    BOOL want_reverse = (NXGOHitY > sc_y + height/2);
    Press (want_reverse, HitWasControl);
}

BOOL SwitchKey::Press (BOOL want_reverse, BOOL lock_it) {
    if (want_reverse) {
	if (Normal)
	    ReportToRelay (Turn->NL, Normal = FALSE);
	ReportToRelay (Turn->RL, Reverse = !Reverse);
    }
    else {
	if (Reverse)
	    ReportToRelay (Turn->RL, Reverse = FALSE);
	ReportToRelay (Turn->NL, Normal = !Normal);
    }

    if (!lock_it && (Normal || Reverse))
	WantMouseUp();

    SetTurnSwkeyFlags();
    return TRUE; /* for now */
}

void SwitchKey::SetTurnSwkeyFlags() {
    Invalidate();
    int newkeys = 0;
    if (Normal)
	newkeys |= TN_AUXKEY_FORCE_NORMAL;
    else if (Reverse)
	newkeys |= TN_AUXKEY_FORCE_REVERSE;
    if (newkeys != Turn->AuxKeyForce)  {
	Turn->AuxKeyForce = (char)newkeys;
	Turn->InvalidateAndTurnouts();
    }
}


Virtual void SwitchKey::UnHit () {
    if (!HitWasControl) {
	if (Reverse)
	    ReportToRelay (Turn->RL, Reverse = FALSE);
	if (Normal)
	    ReportToRelay (Turn->NL, Normal = FALSE);
	SetTurnSwkeyFlags();
    }
}

void SwitchKey::ClearAux() {
    HitWasControl = FALSE;
    UnHit();				/* WOW that was clever!! */
}

static int ClearAllFunarg (GraphicObject * g) {
    ((SwitchKey*) g)->ClearAux();
    return 0;
}

void ClearAllAuxLevers () {
    MapGraphicObjectsOfType (TypeId::SWITCHKEY, ClearAllFunarg);
}

#endif
