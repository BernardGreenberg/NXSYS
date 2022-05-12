#include "windows.h"
#include "xtgtrack.h"
#include "signal.h"
#include "typeid.h"
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
	    MapGraphicObjectsOfType (TypeId::STOP, SSMapper);
	ShowStopPolicy = policy;
	StopsChanging = 1;
	UpdateWindow (G_mainwindow);
	StopsChanging = 0;
    }
}


