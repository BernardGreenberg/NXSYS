#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <string>
#include "STLExtensions.h"

#include "nxsysapp.h"
#include "lyglobal.h"
#include "signal.h"
#include "objid.h"
#include "timers.h"
#include "compat32.h"
#include "brushpen.h"
#include "nxglapi.h"
#include "rlyapi.h"
#include "rlymenu.h"
#include "resource.h"

#include "xtgtrack.h"
#include "lyglobal.h"


extern HFONT Fnt;
extern HPEN SigPen, BgPen;
extern HWND G_mainwindow;
extern long RelayClicks;
extern void TrainSigHook (Signal* g, void * v);
extern Relay * CPB0;

bool SignalRIsMenu = false;  // hacked outside of this on Windows, this helps Mac.

HWND MakeSigWin (Signal * s, int x, int y);

std::string Signal::FormatPlate(TrackSec* ts, const std::string& lights, bool first) {
    if (first) {
        std::string routid;
        int trkno;
        int stano;
        GetPlateData (routid, trkno, stano);
        if (Glb.IRTStyle)
            return FormatString("%d\n%s", 10*stano + trkno, routid.c_str());
        else
            return FormatString("%s%d\n%d", routid.c_str(), trkno, stano);
    }
    else if (XlkgNo > 0 && lights.size() > 2)
        return FormatString("X\n%d", XlkgNo);
    else return("");
    
}

Signal::Signal (int xno, int sno, HeadsArray& headstrings)
{
    TrackSec * ts = nullptr;   // need Version 1, forget it.
    StationNo = sno;
    XlkgNo = xno;
    HG = HVG = DG = DivG = 0;
    MiscG = 0;				/* flags */
    //rdp;

    bool first = true;
    for (const std::string& headstring : headstrings) {
        std::string plate(FormatPlate(ts, headstring, first));
        Heads.push_back(SigHead (headstring, plate));
        first = false;
    }

    /* really should only do GK for interlocked */

    Selected = Fleeted = 0;
    Coding = 0;
    TrainLooking = NULL;
    Window = NULL;
    TStop = NULL;
    PB = PBS = FL = COPB = NULL;
}

std::string Signal::CompactName () {
    int trkno;
    int stano;
    std::string routid;
    GetPlateData (routid, trkno, stano);
    if (Glb.IRTStyle)
        return FormatString("%d/%s", stano*10+trkno, routid.c_str());
    else
	return FormatString("%s%d-%d", routid.c_str(), trkno, stano);
}
    

void Signal::GetPlateData (std::string& routid, int& trkno, int& stano) {

    if (Glb.IRTStyle) {
	trkno = StationNo % 10;
	stano = StationNo / 10;
    }
    else {
	trkno = 0;
	stano = StationNo;
	for (int j = 10000; j >=10; j /=10) {
	    if (StationNo >= j) {
		trkno = StationNo/j;
		stano = StationNo % j;
		break;
	    }
	}					/* KLUUUUDGGEEE ++++++++++ */
    }
    routid = Glb.RouteIdentifier;
}

int Signal::AK_p () {
    if (XlkgNo == 0)
	return 1;
    for (auto& h : Heads)
	if (h.height > 1)
	    return 0;
    /* 1-headed signal; count reds -- 30 June 2001 */

    /* These heuristics succeed until we reach a non-AK'able automatic
       (brand-new or ancient signalling paradigm, e.g., 1652/E at
       Atlantic Avenue, where either some kind of explicit flag will have
       to be supplied, or the TrainSystem will really have to deal
       with verifying stop driving (which will then screw up with
       markers in the call-on VS'ed state) */

    int reds = 0;
    for (char& c : Heads[0].Lights)
	if (c == 'R')
	    reds++;
    return (reds < 2);
}

HBRUSH Signal::GetGKBrush () {
    BOOL yellow = Selected && (HG || (MiscG & SIGF_CO));
    if (Coding == 2)
	if (yellow)
	    return GKYellowBrush;
	else
	    return GKRedBrush;
    else if (Coding == 1)
	return GKOffBrush;
    else if (!Selected)
	return GKOffBrush;
    else if (yellow)
	return GKYellowBrush;
    else 
	return GKRedBrush;
}


void Signal::Invalidate() {
    PSignal->Invalidate();
    UpdateLights();
}

void Signal::UpdateLights() {
    if (Window != NULL) {
#ifdef NXSYSMac
        InvalidateRect(Window,NULL,0);
#else
	HDC dc = GetDC (Window);
	WinDisp (dc, 1);
	ReleaseDC (Window, dc);
#endif
    }
    ReportToHook();
#ifdef NXOGL
    NXGLReportSignalStateChange(this);
#endif
}

void Signal::ReportToHook() {
    if (TrainLooking != NULL)
	TrainSigHook (this, TrainLooking);
}

BOOL Signal::ShouldBeCoding () {
    if (MiscG & SIGF_HARDWIRE_COCLK)
	if (!((MiscG & SIGF_CO) && TStop->Tripping))
	    CLKReporter(FALSE, this);
    return MiscG & SIGF_CODING;
}

void Signal::GKCoder (BOOL state) {
    if (!PSignal->Visible)
	return;
    if (ShouldBeCoding()) {
	Coding = state ? 1 : 2;
#ifdef NXSYSMac
        Invalidate();
#else
	HDC dc = GetDC (G_mainwindow);
	PSIGQUAL Display(dc);
	ReleaseDC (G_mainwindow, dc);
#endif
    }
    else
	Invalidate();
}


BOOL Signal::Cancel () {
    if (PB)
	ReportToRelay (PB, FALSE);
    if (Fleeted)
	Fleet (FALSE);
    if (PBS)
	ReportToRelay (PBS, FALSE);
    return TRUE;
}

void Signal::UnHit () {
    ReportToRelay (PB, FALSE);
}

BOOL Signal::Initiate () {
    if (XlkgNo == 0)
	return FALSE;
    PulseToRelay (PB);
    return Initiated();
}

BOOL Signal::Initiated () {
    return (PBS && RelayState(PBS));
}


BOOL Signal::InitiateClick () {
    if (XlkgNo == 0)
	return FALSE;
    long clicks = RelayClicks;
    BOOL was_selected = Selected;
    PSignal->WantMouseUp();
    ReportToRelay (PB, TRUE);
    if (was_selected)
	/* Whoops - must test for UR exit feature */
	if (RelayClicks < clicks+2)
	    Cancel();
    return TRUE;
}



BOOL Signal::CallOn () {
    if (!RelayUseDefined (COPB))
	return FALSE;
    PulseToRelay (COPB);
    return TRUE;
}

BOOL Signal::ShowFullsigWindow (BOOL ShowNotHide) {

    if (!ShowNotHide) {
	if (Window)
	    ShowWindow (Window, SW_HIDE);
	return TRUE;
    }
	
    if (Window == NULL) {
	POINT p;
	p.x = WPXtoSC (PSignal->wp_x);
	p.y = WPYtoSC (PSignal->wp_y) + 2*(int)(PSignal->Radius*NXGO_Scale);

	ClientToScreen (G_mainwindow, &p);
	Window = MakeSigWin(this, p.x, p.y);
    }
    InvalidateRect (Window, NULL, 0);
    ShowWindow (Window, SW_SHOWNOACTIVATE);

    SetFocus (Window);
    return TRUE;
}
    

void Signal::Hit (int mb) { 
    switch (mb) {
	case (WM_LBUTTONDOWN):
	    InitiateClick();
	    break;
	case WM_NXGO_LBUTTONSHIFT:
	    Fleet (!Fleeted); /* checks FL defined */
	    if (Fleeted && !Selected)
		Initiate();
	    break;
	case WM_NXGO_LBUTTONCONTROL:
	    CallOn();
	    break;
	case WM_NXGO_RBUTTONCONTROL:
	    ContextMenu();
	    break;
        case WM_RBUTTONDOWN:
            if (SignalRIsMenu)
                ContextMenu();
            else
                ShowFullsigWindow (TRUE);
            break;
	default:
                ShowFullsigWindow (TRUE);
	    break;
    }
}

#ifndef TLEDIT

void Signal::EditContextMenu(HMENU m) {
    if (!XlkgNo) {
	DeleteMenu(m, 1, MF_BYPOSITION);
	DeleteMenu(m, ID_INITIATE, MF_BYCOMMAND);
	DeleteMenu(m, ID_CANCEL, MF_BYCOMMAND);
	DeleteMenu(m, ID_VPB, MF_BYCOMMAND);
	DeleteMenu(m, ID_CALL_ON, MF_BYCOMMAND);
	DeleteMenu(m, ID_RESET_APPROACH, MF_BYCOMMAND);
	DeleteMenu(m, ID_TOGGLE_FLEETING, MF_BYCOMMAND);
    }
    else {
	Relay * R;
	if (Initiated()) {
	    EnableMenuItem(m, ID_INITIATE, MF_BYCOMMAND|MF_GRAYED);
	}
	else {
	    EnableMenuItem(m, ID_CANCEL, MF_BYCOMMAND|MF_GRAYED);
	    EnableMenuItem(m, ID_CALL_ON, MF_BYCOMMAND|MF_GRAYED);
	}
	BOOL hasco = RelayUseDefined (COPB);
	if (!hasco) {
	    DeleteMenu(m, ID_CALL_ON, MF_BYCOMMAND);
	    DeleteMenu(m, ID_VPB, MF_BYCOMMAND);
	}
	if (!(XlkgNo && GetRelay2NoCreate (XlkgNo, "AS")))
	    DeleteMenu(m, ID_RESET_APPROACH, MF_BYCOMMAND);
	else
	    if ((R = GetRelay2NoCreate (XlkgNo, "AS"))&&RelayState(R))
		EnableMenuItem(m, ID_RESET_APPROACH,
			       MF_BYCOMMAND|MF_GRAYED);
	if (!(XlkgNo && FL && GetRelay2NoCreate (XlkgNo, "FL")
	      && RelayUseDefined (FL)))
	    DeleteMenu(m, ID_TOGGLE_FLEETING, MF_BYCOMMAND);
	if (hasco && !((MiscG & SIGF_CO) && TStop))
	    EnableMenuItem(m, ID_VPB, MF_BYCOMMAND|MF_GRAYED);
	if (hasco && (MiscG & SIGF_CO))
	    EnableMenuItem(m, ID_CALL_ON, MF_BYCOMMAND);
    }
}

#endif

void Signal::ContextMenu () {
    int id = XlkgNo ? XlkgNo : StationNo;
    int option = PSignal->RunContextMenu (IDR_SIGNAL_CONTEXT_MENU);
    switch (option) {
	case ID_DRAW_RELAY:
	    DrawRelaysForObject (id, "Signal");
	    break;
	case ID_RELAY_QUERY:
	    ShowStateRelaysForObject (id, "Signal");
	    break;
	case IDC_FULL_SIGNAL_DISPLAY:
	    ShowFullsigWindow(TRUE);
	    break;
	case ID_INITIATE:
	    Initiate();
	    break;
	case ID_CALL_ON:
	    CallOn();
	    break;
	case ID_TOGGLE_FLEETING:
	    Fleet (!Fleeted); /* checks FL defined */
	    break;
	case ID_CANCEL:
	    Cancel();
	    break;
	case ID_VPB:
	    if (TStop)
		TStop->PressStopPB();
	    break;
	case ID_RESET_APPROACH:
	    ResetApproach();
	    break;
    }
}

void Signal::HReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    g->HG = state;
    g->Invalidate();
}

void Signal::DReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    g->DG = state;
    g->UpdateLights();
}

void Signal::DivReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    g->DivG = state;
    g->UpdateLights();
}


void Signal::RReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    g->Selected = state;
    g->Invalidate();
}

void Signal::FlagReporterCommon (unsigned int flag, BOOL state) {
    if (state)
	MiscG |= flag;
    else
	MiscG &= ~flag;
    UpdateLights();
}
   

void Signal::SKReporter(BOOL state, void* v) {
    ((Signal*)v)->FlagReporterCommon (SIGF_S, state);
}

void Signal::DKReporter(BOOL state, void* v) {
    ((Signal*)v)->FlagReporterCommon (SIGF_D, state);
}

void Signal::STRReporter(BOOL state, void* v) {
    ((Signal*)v)->FlagReporterCommon (SIGF_STR, state);
}


void Signal::CLKReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    if (state) {
	g->MiscG |= SIGF_CODING;
	NXCoder (g, Signal::GKCoderReporter);
    }
    else {
	if (g->MiscG & SIGF_CODING)
	    KillOneCoder (g);
	g->MiscG &= ~SIGF_CODING;
	g->Coding = 0;
    }
    g->Invalidate();
}


void Signal::COReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    if (state)
	g->MiscG |= SIGF_CO;
    else 
	g->MiscG &= ~SIGF_CO;
    if (g->MiscG & SIGF_HARDWIRE_COCLK) /* older implementation. */
	CLKReporter (state, g);
    g->Invalidate();
}

void Signal::LunarReporter(BOOL state, void* v) {
    ((Signal*)v)->FlagReporterCommon (SIGF_LUNAR, state);
}


void Signal::LunarWhenRedReporter(BOOL state, void* v) {
    Signal * g = (Signal *) v;
    if (state)
	g->MiscG |= SIGF_LUNAR_WHEN_RED;
    else 
	g->MiscG &= ~SIGF_LUNAR_WHEN_RED;
    if (g->ComputeState() == 'R')
	g->UpdateLights();
}

void Signal::GKCoderReporter  (void* v, BOOL state) {
    ((Signal *) v) ->GKCoder(state);
}

BOOL Signal::Fleet (BOOL onoff) {
    if (!RelayUseDefined (FL))
	return FALSE;

    if (Fleeted != onoff) {
	ReportToRelay (FL, Fleeted = onoff);
	Invalidate();
    }
    return TRUE;
}

int DropperFunarg (GraphicObject * go) {
    ((PanelSignal *) go)->Sig->Cancel();
    return 0;
}


void DropAllSignals () {
    MapGraphicObjectsOfType (ID_SIGNAL, DropperFunarg);
    if (CPB0 != NULL)
	PulseToRelay (CPB0);
}


void Signal::SigStateExt (Signal * s) {
    s->ComputeState();
}

BOOL Signal::ResetApproach () {
    if (!XlkgNo)
	return FALSE;
    // Doesn't have to be terribly efficient.
    Relay* U = GetRelay2NoCreate (XlkgNo, "U");
    if (U) {
	PulseToRelay (U);
	return TRUE;
    }
    else return FALSE;
}

static int FSDDestructor(GraphicObject * g) {
    PanelSignal*P = (PanelSignal*)g;
    Signal*S = P->Sig;
    if (S->Window != NULL) {
        ShowWindow(S->Window, SW_HIDE);
        DestroyWindow(S->Window);
        S->Window = NULL;
    }
    return 0;
}

static int FSDCloser(GraphicObject * g) {
    PanelSignal*P = (PanelSignal*)g;
    Signal*S = P->Sig;
    if (S->Window != NULL) {
        ShowWindow(S->Window, SW_HIDE);
    }
    return 0;
}

/* might work on Windows even */
/* This strategy is arguably better than maintaining a registry of FSW's. */
void CloseAllFSWs (bool destroy) {
    if (destroy)
        MapGraphicObjectsOfType(ID_SIGNAL, FSDDestructor);
    else
        //but for a closer walk with thee, o Kali destructress . . .
        MapGraphicObjectsOfType(ID_SIGNAL, FSDCloser);
}
