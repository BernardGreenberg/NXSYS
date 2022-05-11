#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "xtgtrack.h"
#include "xturnout.h"

#include "trafficlever.h"
#include "objid.h"
#include "ssdlg.h"
#include "brushpen.h"
#include "compat32.h"
#include "rlyapi.h"
#include "lyglobal.h"
#include "PolyKludge.h"
#include <cassert>
#ifndef M_PI
#define M_PI  3.14159265358979323846264
#endif
#include "nxsysapp.h"

#define TL_TRISTATE 1
#define NUM_DT_OPTS DT_CENTER |DT_SINGLELINE |DT_NOCLIP

static int BottomMargin= 2;
static int NumHeight;
static int KnobRadius, IndicatorRadius, IndicatorOffset, IndicOffsAdj;
static int Fuzz = 2;
static int BigRadius;
static double KnobAngle;

static HBRUSH BrightGray;
static HPEN LinePen;

#ifndef Virtual
#define Virtual
#endif

TrafficLever::TrafficLever (int xno, WP_cord p_wpx, WP_cord p_wpy,
			   int right_normal) :
   Reverse(FALSE), Normal(TRUE)
{
    /* This is an optional route parameter, not a traffic lever parameter, in order
     to keep compatibly-coded interlockings and NXSYS versions forward-and-backward
     compatible. */

    TriState = (bool)Glb.TrafficLeversTristate;


    wp_x = p_wpx;
    wp_y = p_wpy;
    wp_limits.left   = -IndicatorOffset-IndicatorRadius-Fuzz;
    wp_limits.right  = +IndicatorOffset+IndicatorRadius+Fuzz;
    wp_limits.top    = -KnobRadius-Fuzz;
    wp_limits.bottom = KnobRadius + NumHeight + BottomMargin+Fuzz;

    for (auto& tli : Indicators)
	tli.Lever = this;
    SetNormalReverseStatus (right_normal);
    Normal = !TriState;
    Reverse = FALSE;
    NL = RL = NULL;
    SetXlkgNo (xno);
    KnobLeft = KnobRight = false;
}

int TrafficLeverIndicator::rad;
double TrafficLeverIndicator::doff;
int TrafficLeverIndicator::arht;

void TrafficLeverIndicator::DataInit() {
    rad = (int)ceil(IndicatorRadius * NXGO_Scale);
    doff = ceil(rad*.60 + 1);
    arht = (int)ceil(rad/4.0);
}

void TrafficLever::SetNormalReverseStatus (int right_normal) {
    NormalIndex = right_normal ? TRAFLEV_RIGHT : TRAFLEV_LEFT;
    ReverseIndex = 1 - NormalIndex;
}

void TrafficLever::SetXlkgNo (int xno) {
    XlkgNo = xno;
    NumString = std::to_string(xno);
}

void InitTrafficLeverData () {
    int gu = GU2;			/* expected to be about 3 */
    KnobRadius = 6*gu;
    BigRadius = (int)ceil(KnobRadius * NXGO_Scale);
    IndicatorRadius = 3*gu;
    IndicatorOffset = 2*KnobRadius;
    IndicOffsAdj = (int)ceil(IndicatorOffset * NXGO_Scale);

    KnobAngle = M_PI/4;

    BrightGray = CreateSBrush(RGB(192,192,192));
    LinePen = CreateSPen(1, RGB(0,0,0));
    HDC hDC = GetDC (G_mainwindow);
    SelectObject (hDC, Fnt);
    RECT r{};
    r.top = r.left = 0;
    DrawText (hDC, "1", 1, &r, NUM_DT_OPTS | DT_CALCRECT);
    NumHeight = r.bottom;
    ReleaseDC (G_mainwindow, hDC);
    TrafficLeverIndicator::DataInit();
}

#ifndef TLEDIT
void TrafficLever::InitState() {
    if (TriState)
        PulseToRelay(NL);
    else
        ReportToRelay(NL, TRUE);
}
#endif

Virtual void TrafficLever::Display (HDC hdc) {

    HBRUSH brush = BrightGray;
#ifdef TLEDIT
    if (Selected)
	brush = GKGreenBrush;
#endif

    int rad = BigRadius;
    RECT tr{};
    tr.top = sc_y + rad + BottomMargin;
    tr.bottom = tr.top + NumHeight;
    tr.left = sc_x - rad;
    tr.right = sc_x + rad;
    DrawText (hdc, NumString.c_str(), (int)NumString.size(), &tr, NUM_DT_OPTS);

    SelectObject (hdc, brush);
    SelectObject (hdc, (HBRUSH)GetStockObject(NULL_PEN)); /* moved up 12 May 2001 */
    Ellipse(hdc, sc_x - rad, sc_y - rad, sc_x + rad, sc_y + rad);

    DrawKnob(hdc);

    for (auto& indic : Indicators)
        indic.Draw(hdc, sc_x, sc_y);
}

void TrafficLeverIndicator::Draw(HDC hdc, int l_xcen, int y) {

    HBRUSH brush;
    if (White)
        brush = GKWhiteBrush;
    else if (Red)
        brush = GKRedBrush;
    else
        brush = GKOffBrush;
    SelectObject (hdc, brush);
    int x = l_xcen + IndicOffsAdj*PlusMinusOne;
    Ellipse (hdc, x - rad, y - rad, x + rad, y + rad);
    SelectObject (hdc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    int rectht = arht;
    int xvleft = (int)(x + PlusMinusOne*floor(doff*.6)); // "virtual" left...
    int xvright = (int)(x - PlusMinusOne*doff);
    Rectangle (hdc, xvleft, y - rectht+1, xvright, y + rectht-1);
    POINT points[3]{};
    points[0].x = (int)(xvleft + PlusMinusOne*(floor(doff/2)));
    points[0].y = y;
    points[1].x = xvleft;
    points[1].y = y + arht;
    points[2].x = xvleft;
    points[2].y = y - arht;
    Polygon(hdc, points, 3);
}

void TrafficLever::LocateKnobPoint(double radius, double angle, int& x, int& y) {
    x = (int)(sc_x + radius*sin(angle));
    y = (int)(sc_y - radius*cos(angle));
}

void TrafficLever::DrawKnob(HDC hdc) {

    double angle = 0.0;
#if !TLEDIT
    if (RelayState(NL)) angle = KnobAngle;
    else if (RelayState(RL)) angle = -KnobAngle;
    else angle = 0.0;
#endif
    if (KnobLeft) angle = -KnobAngle;
    else if (KnobRight) angle = KnobAngle;
    else if (NormalIndex == TRAFLEV_LEFT)
        angle = - angle;

    SelectObject (hdc, (HBRUSH)GetStockObject(BLACK_BRUSH));
    int trad = (int)(KnobRadius*.25*NXGO_Scale + .8);
    int irad = BigRadius - trad;
    int xxcen, yycen;
    LocateKnobPoint(irad, angle, xxcen, yycen);
    Ellipse (hdc, xxcen - trad, yycen - trad, xxcen + trad, yycen + trad);
    double botang = angle + M_PI;
    int xbot, ybot;
    LocateKnobPoint(BigRadius, botang, xbot, ybot);
    SelectObject(hdc, LinePen);
#if 0
    float halfang = asin(float(trad)/float(2*BigRadius - trad));
    float radprojp = BigRadius - trad - trad*sin(halfang);
    float xdisptanp = trad * cos(halfang);
    float ctrangtap = atan2(xdisptanp, radprojp);
    float tradhyp =  sqrt(pow(xdisptanp,2) + pow(radprojp,2));
#else
    double PZ = 2*BigRadius - trad;
    // PU::PT = PT::PZ  (trad = PT)
    double PU = trad*(trad/PZ);
    double UO = BigRadius - trad - PU;
    double UZ = UO + BigRadius;
    double TU = sqrt(PU*UZ); // rt tri PTZ altitude
    double ctrangtap = atan2(TU, UO);
    double tradhyp =  hypot(TU, UO);
#endif
    int tpx, tpy;
    LocateKnobPoint(tradhyp, angle+ctrangtap, tpx, tpy);
    MoveTo(hdc, xbot, ybot);
    LineTo(hdc, tpx, tpy);
    LocateKnobPoint(tradhyp, angle-ctrangtap, tpx, tpy);
    MoveTo(hdc, xbot, ybot);
    LineTo(hdc, tpx, tpy);
}

Virtual ObjId TrafficLever::TypeID() {
    return ObjId::TRAFFICLEVER;
}

Virtual int TrafficLever::ObjIDp(long id) {
    return XlkgNo == id;
}

#ifndef TLEDIT
Virtual void TrafficLever::Hit (int mb) {

    if (TriState) {
        WantMouseUp();
        bool left = (NormalIndex == TRAFLEV_LEFT);
        if (NXGOHitX < sc_x) {
            ReportToRelay(left ? NL : RL, true);
            KnobLeft = true;
        }
        else {
            ReportToRelay(left ? RL : NL, true);
            KnobRight = true;
        }
        Invalidate();
    }
    else
        Throw (!Reverse);
}

Virtual void TrafficLever::UnHit () {
    assert(TriState);
    KnobLeft = KnobRight = false;
    ReportToRelay(NL, false);
    ReportToRelay(RL, false);
    Invalidate();
}

BOOL TrafficLever::Throw (BOOL want_reverse) {
    assert (!TriState);
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
