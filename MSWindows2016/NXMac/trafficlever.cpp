#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef NXV2
#include "xtgtrack.h"
#include "xturnout.h"
#else
#include "track.h"
#include "lyglobal.h"
#endif
#include "trafficlever.h"
#include "objid.h"
#include "ssdlg.h"
#include "brushpen.h"
#include "compat32.h"
#include "rlyapi.h"
#include "PolyKludge.h"

#include "nxsysapp.h"

#define NUM_DT_OPTS DT_CENTER |DT_SINGLELINE |DT_NOCLIP

static int BottomMargin= 2;
static int NumHeight;
static int KnobRadius, IndicatorRadius, IndicatorOffset;
static int Fuzz = 2;
static HBRUSH BrightGray;

#ifndef Virtual
#define Virtual
#endif

#ifndef NXV2
#ifndef TLEDIT
#ifndef NXV1
#define NXV1
#endif
#endif
#endif

#ifndef TLEDIT
#if 0
void TrafficLever::LSReporter (BOOL state, void * v) {
    ((TrafficLever *)v)->LSReport(state);
}

void TrafficLever::LSReport (BOOL state) {
    LSState = state;
    Turn->LSReporter (state, Turn);
    Invalidate();
}
#endif
#endif

TrafficLever::TrafficLever (int xno, WP_cord p_wpx, WP_cord p_wpy,
			   int right_normal) :
   Reverse(FALSE), Normal(TRUE)
{
    wp_x = p_wpx;
    wp_y = p_wpy;
    wp_limits.left   = -IndicatorOffset-IndicatorRadius-Fuzz;
    wp_limits.right  = +IndicatorOffset+IndicatorRadius+Fuzz;
    wp_limits.top = -KnobRadius-Fuzz;
    wp_limits.bottom = KnobRadius + NumHeight + BottomMargin+Fuzz;

    for (int i = 0; i < 2; i++) {
	TrafficLeverIndicator * tlip = &Indicators[i];
	tlip->White = tlip->Coding = tlip->Red = FALSE;
	tlip->MyIndex = i;
	tlip->Lever = this;
    }
    SetNormalReverseStatus (right_normal);
    Normal = TRUE;
    Reverse = FALSE;
    NL = RL = NULL;
    SetXlkgNo (xno);
}

void TrafficLever::SetNormalReverseStatus (int right_normal) {
    NormalIndex = right_normal ? TRAFLEV_RIGHT : TRAFLEV_LEFT;
    ReverseIndex = 1 - NormalIndex;
}

void TrafficLever::SetXlkgNo (int xno) {
    XlkgNo = xno;
    sprintf (NumStr, "%d", xno);
    NumStrLen = (int) strlen(NumStr);
}

#ifdef NXV1


static void CreateTrafficLever (int xno) {
    xno;
}

#endif

void InitTrafficLeverData () {
    int gu = GU2;			/* expected to be about 3 */ 
    KnobRadius = 6*gu;
    IndicatorRadius = 3*gu;
    IndicatorOffset = 2*KnobRadius;

    BrightGray = CreateSBrush(RGB(192,192,192));
    HDC hDC = GetDC (G_mainwindow);
    SelectObject (hDC, Fnt);
    RECT r;
    r.top = r.left = 0;
    DrawText (hDC, "1", 1, &r, NUM_DT_OPTS | DT_CALCRECT);
    NumHeight = r.bottom;
    ReleaseDC (G_mainwindow, hDC);
}

#ifdef NXV1
void CreateTrafficLevers () {

    InitTrafficLeverData();

}
#endif


Virtual void TrafficLever::Display (HDC hdc) {

    HBRUSH brush = BrightGray;
#ifdef TLEDIT
    if (Selected)
	brush = GKGreenBrush;
#endif

    double bigrad = KnobRadius * NXGO_Scale + .8;
    double rad = bigrad;
    RECT tr;
    tr.top = (int)(sc_y + rad + BottomMargin);
    tr.bottom = (int)(tr.top + NumHeight);
    tr.left = (int)(sc_x - rad);
    tr.right = (int)(sc_x + rad);
    DrawText (hdc, NumStr, NumStrLen, &tr, NUM_DT_OPTS);


    int xcen = sc_x;
    int ycen = sc_y;
    SelectObject (hdc, brush);
    SelectObject (hdc, (HBRUSH)GetStockObject(NULL_PEN)); /* moved up 12 May 2001 */

    Ellipse(hdc, (int)(xcen - rad), (int)(ycen - rad), (int)(xcen + rad), (int)(ycen + rad));

    double offs = IndicatorOffset * NXGO_Scale + .8;
    rad = IndicatorRadius * NXGO_Scale + .8;
    double doff = rad*.60;
    double arht = rad/8.0+1;
    HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
    for (int i = 0; i < 2; i++) {
		TrafficLeverIndicator * tlip = &Indicators[i];
	xcen = (int)(sc_x - offs + 2*offs*i);
	if (tlip->White)
	    brush = GKWhiteBrush;
	else if (tlip->Red)
	    brush = GKRedBrush;
	else brush = GKOffBrush;
	SelectObject (hdc, brush);
	Ellipse (hdc, (int)(xcen - rad), (int)(ycen - rad), (int)(xcen + rad), (int)(ycen + rad));
	SelectObject (hdc, black);
	Rectangle (hdc, (int)(xcen - doff), (int)(ycen-arht+1), (int)(xcen + doff), (int)(ycen+arht-1));
	POINT points[3];
	double koff = -1 + 2*i;
	points[0].x = (int)(xcen + koff*(doff+1));
	points[0].y = (int)ycen;
	points[1].x = points[2].x = (int)xcen + (int)(koff*doff*.5 + .5);
	points[1].y = (int)(ycen+2*arht);
	points[2].y = (int)(ycen-2*arht);
	Polygon(hdc, points, 3);
    }
    /* knob rim */
    SelectObject (hdc, black);
    int trad = (int)(KnobRadius*.25*NXGO_Scale + .8);
    
    int minusp = (Reverse ? 1 : -1);
    if (NormalIndex == TRAFLEV_RIGHT)
	minusp *= -1;
    xcen = (int)(sc_x + (bigrad - trad)*minusp);
    Ellipse (hdc, xcen - trad, ycen - trad, xcen + trad, ycen + trad);
}

Virtual int TrafficLever::TypeID() {
    return ID_TRAFFICLEVER;
}

Virtual int TrafficLever::ObjIDp(long id) {
    return XlkgNo == id;
}

#ifndef TLEDIT
Virtual void TrafficLever::Hit (int mb) {

#if 0
    int height = Height;
    if (NXGO_Scale < 1.0)
	height = (int) (height*NXGO_Scale);

    if (NXGOHitY > sc_y + height)
	return;
#endif
    Throw (!Reverse);
}

BOOL TrafficLever::Throw (BOOL want_reverse) {
    if (want_reverse != Reverse) {
	Reverse = want_reverse;
	if (Reverse) {
	    ReportToRelay (NL, FALSE);
	    ReportToRelay (RL, TRUE);
	}
	else {
	    ReportToRelay (RL, FALSE);
	    ReportToRelay (NL, TRUE);
	}
	Invalidate();
    }
    return TRUE;
}

void TrafficLever::ClearAux() {
    Throw(FALSE);
}

static int NormalAllFunarg (GraphicObject * g) {
    ((TrafficLever*) g)->ClearAux();
    return 0;
}

void NormalAllTrafficLevers () {
    MapGraphicObjectsOfType (ID_TRAFFICLEVER, NormalAllFunarg);
}

void TrafficLeverIndicator::WhiteReporter(BOOL state, void* v) {
    ((TrafficLeverIndicator*)v)->SetWhite(state);
}

void TrafficLeverIndicator::RedReporter(BOOL state, void* v) {
    ((TrafficLeverIndicator*)v)->SetRed(state);
}

void TrafficLeverIndicator::SetWhite (BOOL on) {
    if (White != on)
	Lever->Invalidate();
    White = on;
}

void TrafficLeverIndicator::SetRed (BOOL on) {
    if (Red != on)
	Lever->Invalidate();
    Red = on;
}

#endif
