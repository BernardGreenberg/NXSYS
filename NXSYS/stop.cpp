#include "windows.h"
#ifdef XTG
#include "xtgtrack.h"
#else
#include "track.h"
#endif
#include "signal.h"
#include "objid.h"
#include "ssdlg.h"
#include "brushpen.h"
#include "timers.h"
#include "compat32.h"
#include "nxglapi.h"
#include "rlyapi.h"

extern HWND G_mainwindow;

int ShowStopPolicy = SHOW_STOPS_RED;
int StopsChanging = 0;

#define STOP_CODE_CLICKS_MAX 6

#define STOP_BAR_WIDTH 5
#define STOP_STEM_WIDTH 4

#ifndef XTG
#include "stop.h"


Stop::Stop (Signal * g) {
    Sig = g;
    Tripping = 1;
    RVP = NVP = VPB = NULL;
    g->TStop = this;
    wp_y = g->ForwardTS->wp_y;
    wp_x = g->ForwardTS->wp_x;

    wp_limits.left = - 2*GU1;
    wp_limits.right = 2*GU1;
    if (g->Southp) {
	wp_x += g->ForwardTS->wp_len;
	wp_x -= GU1;
	wp_limits.bottom = -GU1;
	wp_limits.top = wp_limits.bottom-5*GU1;
    }
    else {
	wp_x += GU1;
	wp_limits.top = GU1;
	wp_limits.bottom = wp_limits.top+5*GU1;
    }
    coding = 0;
    code_clicks = 0;
}


int Stop::TypeID() {return ID_STOP;}
int Stop::ObjIDp(long x) {return Sig->ObjIDp(x);};

#endif

bool Stop::MouseSensitive () {return false;}

#include <stdio.h>
void Stop::VReporter(BOOL state) {

    Tripping = !state;
 
    if (NVP != NULL)
	ReportToRelay(NVP, FALSE);
    if (RVP != NULL)
	ReportToRelay(RVP, FALSE);
    code_clicks = 0;
    if (!coding) {
	coding = 1;
	NXFastCoder (this, StopCoderReporter);
    }
    Invalidate();
    Sig->UpdateStop();
}

void Stop::VReporterReporter(BOOL state, void* v) {
    ((Stop *)v)->VReporter(state);
}

int Stop::PressStopPB() {
    if (RelayUseDefined (VPB)) {
	PulseToRelay (VPB);
	return 1;
    }
    return 0;
}

#ifndef XTG

void Stop::Display (HDC dc) {
    HGDIOBJ brush;

    if (ShowStopPolicy == SHOW_STOPS_NEVER && !StopsChanging)
	return;

    brush = Tripping ? GKRedBrush : GKYellowBrush;

    if (ShowStopPolicy == SHOW_STOPS_NEVER)
	brush = GetStockObject(BLACK_BRUSH);
    else if (coding)
	if (code_clicks & 1)
	    brush = GetStockObject(BLACK_BRUSH);
	else;
    else
	if (ShowStopPolicy == SHOW_STOPS_RED && !Tripping)
	    brush = GetStockObject(BLACK_BRUSH);

    SelectObject (dc, NullPen);
    SelectObject (dc, brush);

    POINT point[3];
    point[0].x = sc_limits.left;
    point[1].x = sc_limits.right;
    point[2].x = (sc_limits.left + sc_limits.right)/2;

    if (Sig->Southp) {
	point[0].y = sc_limits.bottom;
	point[1].y = sc_limits.bottom;
	point[2].y = sc_limits.top;
    }
    else {
	point[0].y = sc_limits.top;
	point[1].y = sc_limits.top;
	point[2].y = sc_limits.bottom;
    }
    Polygon (dc, point, 3);
}

#endif

void Stop::CodingDisplay() {
    if (ShowStopPolicy != SHOW_STOPS_NEVER) {
#ifdef NXSYSMac
        Invalidate();
#else
	HDC dc = GetDC (G_mainwindow);
	Display (dc);
	ReleaseDC (G_mainwindow, dc);
#endif
    }
    Sig->UpdateStop();
}


void Stop::StopCoder (BOOL /*codestate*/) {
    if (!coding) {
	KillOneFastCoder(this);
	return;
    }

    if (code_clicks++ >= STOP_CODE_CLICKS_MAX) {
	KillOneFastCoder(this);
	code_clicks = 0;
	coding = 0;
	CodingDisplay();
#ifdef NXOGL
	NXGLReportSignalStateChange (Sig);
#endif
	if (Tripping)
	    if (NVP)
		ReportToRelay(NVP, TRUE);
	    else;
	else 
	    if (RVP)
		ReportToRelay(RVP, TRUE);
    }
    else
	CodingDisplay();
}




void Stop::StopCoderReporter(void* v, BOOL codestate) {
    Stop * s = (Stop *) v;		/* v, hahaha */  /* (i.e., "v" for "void" and "valve" (2014)) */
    s->StopCoder(codestate);
}



void  Stop::SigWinDisplay (HDC hDC, RECT * rp) {



#ifdef NXSYSMac
    HBRUSH stopstem = GKYellowBrush;
#else // better policy all around -- perhaps win, too.
    HBRUSH stopstem = (HBRUSH)GetStockObject (BLACK_BRUSH);
    HBRUSH white = (HBRUSH)GetStockObject (WHITE_BRUSH);
    FillRect (hDC, rp, white);
#endif

    int points = coding ? code_clicks : STOP_CODE_CLICKS_MAX;
    if (!Tripping)
	points = STOP_CODE_CLICKS_MAX - points;
    RECT bar = *rp;
    /* leave down stop lying on bottom of box ... */
    int tot_height = bar.bottom - bar.top - STOP_BAR_WIDTH; 
    bar.top += ((STOP_CODE_CLICKS_MAX - points)*tot_height)
	       /STOP_CODE_CLICKS_MAX;
    bar.bottom = bar.top + STOP_BAR_WIDTH;
    FillRect (hDC, &bar, GKYellowBrush);
    MoveTo (hDC, bar.left, bar.top);
    LineTo (hDC, bar.right-1, bar.top);
    LineTo (hDC, bar.right-1, bar.bottom);
    LineTo (hDC, bar.left, bar.bottom);
    LineTo (hDC, bar.left, bar.top);
    RECT stem = *rp;
    stem.top = bar.bottom;
    stem.left = (stem.left + stem.right)/2 - STOP_STEM_WIDTH/2;
    stem.right = stem.left + STOP_STEM_WIDTH;
    FillRect (hDC, &stem, stopstem);
    FillRect (hDC, &stem, GKYellowBrush);
}


static int SSMapper (GraphicObject * go) {
    if (go->Visible)
	go->Invalidate();
    return 0;
}


void ImplementShowStopPolicy (int policy){
    if (policy > 0) {
	if (policy != ShowStopPolicy)
	    MapGraphicObjectsOfType (ID_STOP, SSMapper);
	ShowStopPolicy = policy;
	StopsChanging = 1;
	UpdateWindow (G_mainwindow);
	StopsChanging = 0;
    }
}


