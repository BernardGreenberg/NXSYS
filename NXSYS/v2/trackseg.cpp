#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <cassert>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "typeid.h"
#include "brushpen.h"
#include "salvager.hpp"
#include "undo.h"

#include "pival.h"
#include "NXSYSMinMax.h"

#ifdef TLEDIT
#include "assignid.h"
#include "tledit.h"
#define SNAP_DELTA (2*Track_Width_Delta)
#else
#include "nxsysapp.h"
#include "resource.h"
#include "rlymenu.h"
#include "signal.h"
#include "xturnout.h"
#include "rlyapi.h"
#include "traindcl.h"

#define SNAP_DELTA Track_Width_Delta
#endif


#ifdef NXSYSMac
extern bool GlobalOfferingChooseTrack;
void GlobalChooseTrackHandler(void*);
#endif
static float fBlipLength;

TrackSegEnd::TrackSegEnd() {
#ifdef REALLY_NXSYS
    Next = NextIfSwitchThrown = NULL;
    FacingSwitch = NULL;
#endif
    SignalProtectingEntrance = NULL;
    ExLight = NULL;
    Joint = NULL;
};

TrackSeg::TrackSeg (WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2){
    fBlipLength = (float) (2*GU2);

    Circuit = NULL;
    Routed = TRUE;
    TrainCount = 0;
#ifdef REALLY_NXSYS
    OwningTurnout = NULL;
    RWLength = -1.0f;			/* "not computed" */
#endif

    Align (wpx1, wpy1, wpx2, wpy2);
    MakeSelfVisible();

}
 
void TrackSeg::Align (WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2){
    WP_cord deltay = wpy2-wpy1;
    WP_cord deltax = wpx2-wpx1;

    double theta = atan2 ((double) deltay, (double) deltax);
    CosTheta = (float) cos(theta);
    SinTheta = (float) sin(theta);

    /* should really be rw units ? */
    Length = (float) sqrt (deltax*deltax + deltay*deltay);

    GraphicBlips = (int) ((Length-1*fBlipLength)/Track_Seg_Len);

    WP_cord higher_y  = NXMIN(wpy1, wpy2);
    WP_cord lower_y   = NXMAX(wpy1, wpy2);
    WP_cord lefter_x  = NXMIN(wpx1, wpx2);
    WP_cord righter_x = NXMAX(wpx1, wpx2);

    Ends[0].wpx = wpx1;
    Ends[1].wpx = wpx2;
    Ends[0].wpy = wpy1;
    Ends[1].wpy = wpy2;

    wp_x = lefter_x;
    wp_y = higher_y;
    int d = 2*Track_Width_Delta;
    wp_limits.left = -d;
    wp_limits.right = int(righter_x - lefter_x + d);
    wp_limits.top =  - d;
    wp_limits.bottom = int((lower_y - higher_y) + d);

    Ends[0].Reposition();
    Ends[1].Reposition();
}
#include "signal.h"


TrackSegEnd& TrackSeg::GetEnd(TSEX endx) {
    if (endx == TSEX::E0)
        return Ends[0];
    else if (endx == TSEX::E1)
        return Ends[1];
    else
        assert("Invalid segment end index.");
    return Ends[0];
}

TrackSegEnd& TrackSeg::GetOtherEnd(TSEX endx) {
    if (endx == TSEX::E0)
        return Ends[1];
    else if (endx == TSEX::E1)
        return Ends[0];
    else
        assert("Invalid segment end index.");
    return Ends[0];
}

TSEX TrackSeg::FindEndIndex(TrackJoint * tj) {
    if (tj == Ends[0].Joint)
        return TSEX::E0;
    else if (tj == Ends[1].Joint)
        return TSEX::E1;
    else
        return TSEX::NOTFOUND;
}

void TrackSegEnd::Reposition () {
    if (SignalProtectingEntrance)
	SignalProtectingEntrance->PSignal->Reposition();
    if (ExLight)
	ExLight->Reposition();
}

void TrackSeg::Align () {
    Align (Ends[0].wpx, Ends[0].wpy, Ends[1].wpx, Ends[1].wpy);
}

WPPOINT TrackSeg::WPPoint() {
    return WPPOINT((Ends[0].wpx + Ends[1].wpx)/2, (Ends[0].wpy + Ends[1].wpy)/2);
}

Virtual void TrackSeg::Display (HDC dc) {
    /* figure out correct state from track sec and containing switch */
    DisplayInState (dc, 0);
}

int RoundSigned (double f) {
    if (f < 0.0)
	return (int)(f - 0.5);
    else
	return (int)(f + 0.5);
}

#ifdef REALLY_NXSYS
BOOL TrackSegEnd::InsulatedP () {
    if (Joint && Joint->TSCount == 3)
	return FALSE;			/* no insulated switches today... */
    if (Next == NULL)
	return TRUE;
    return (Next->Circuit !=
	    Next->GetEnd(EndIndexNormal).Next->Circuit);
}

void TrackSegEnd::OffExitLight () {
    if (ExLight)
	ExLight->Off();
}
#endif

void TrackSeg::GetGraphicsCoords (int ex, int& x, int& y) {
    int si = SectionInterstice*3;
    int flip = (ex == 0) ? 1 : -1;
    x = WPXtoSC(Ends[ex].wpx);
    y = WPYtoSC(Ends[ex].wpy);
    if (Ends[ex].InsulatedP()) {
	x += RoundSigned(si*CosTheta)*flip;
	y += RoundSigned(si*SinTheta)*flip;
    }
}

void TrackSeg::DisplayInState (HDC dc, int control) {

    int scx1, scy1, scx2, scy2;
    GetGraphicsCoords(0, scx1, scy1);
    GetGraphicsCoords(1, scx2, scy2);

#ifdef TLEDIT
    SelectObject (dc, Selected ? HighlightTrackPen : TrackPen);
#else
    SelectObject (dc, TrackPen);
#endif
    MoveTo (dc, scx1, scy1);
    LineTo (dc, scx2, scy2);
    
    HPEN pen;
    if (!Routed || !Circuit)
	pen = NULL;
    else if (Circuit->Occupied)
	pen = TrackOccupiedPen;
    else if (Circuit->Routed)
	pen = TrackRoutedPen;
#ifdef REALLY_NXSYS
    else if (OwningTurnout && OwningTurnout->Thrown)
	pen = TrackDftPenThrown;
#endif
    else 
	pen = NULL;

    if (pen) {
	double  incXSegLen = Track_Seg_Len*CosTheta*NXGO_Scale,
	    incYSegLen = Track_Seg_Len*SinTheta*NXGO_Scale,
	    incXBlipLen = fBlipLength*CosTheta*NXGO_Scale,
	    incYBlipLen = fBlipLength*SinTheta*NXGO_Scale;

	SelectObject(dc, pen);
	double xf = scx1 + incXBlipLen, yf = scy1 + incYBlipLen;
	for (int step = 0; step < GraphicBlips; step++) {
	    int ixf = (int) (xf + .5);
	    int iyf = (int) (yf + .5);
	    int ixdest = (int) (xf + incXBlipLen + .5);
	    int iydest = (int) (yf + incYBlipLen + .5);
	    MoveTo (dc, ixf, iyf);
	    LineTo (dc, ixdest, iydest);
	    xf += incXSegLen;
	    yf += incYSegLen;
	}
    }

//    if (OwningTurnout && (OwningTurnout->AuxKeyForce))
//	OwningTurnout->InvalidateAndTurnouts();
    if (Ends[0].ExLight)
	Ends[0].ExLight->Display(dc);
    if (Ends[1].ExLight)
	Ends[1].ExLight->Display(dc);
}


void TrackSeg::Split (WP_cord wpx1, WP_cord wpy1, TrackJoint * tj, TrackSeg* new_seg) {
 
    /*  Ends[0]            this                       Ends[1] */
    /*  Ends[0]  this      Ends[1] TJ [ENDS[0]  nts   Ends[1]] */

    TrackSeg * nts = new_seg;
    if (!nts)
        nts = new TrackSeg(Ends[0].wpx, Ends[0].wpy, Ends[1].wpx, Ends[1].wpy);
    nts->Align (wpx1, wpy1, Ends[1].wpx, Ends[1].wpy);
    nts->Ends[1] = Ends[1];
    if (Ends[1].SignalProtectingEntrance) {
	Ends[1].SignalProtectingEntrance = NULL;
	nts->Ends[1].SignalProtectingEntrance->PSignal->Seg = nts;
	nts->Ends[1].SignalProtectingEntrance->PSignal->Reposition();
    }
    if (Ends[1].ExLight) {  // 5-16-2022 bug found, fixed by this {}.
        Ends[1].ExLight->Seg = nts;
        Ends[1].ExLight = NULL;
    }
    Align (Ends[0].wpx, Ends[0].wpy, wpx1, wpy1);
    nts->Ends[1].Joint = Ends[1].Joint;
    for (int j = 0; j < Ends[1].Joint->TSCount; j++)
	if (Ends[1].Joint->TSA[j] == this)
	    Ends[1].Joint->TSA[j] = nts;
    Ends[1].Joint = nts->Ends[0].Joint = tj;
    if (Circuit)
	Circuit->AddSeg(nts);
    nts->GetVisible();
    Invalidate();
    nts->Invalidate();
    tj->AddBranch(this);
    tj->AddBranch(nts);
#if TLEDIT
    if (!new_seg)
        Undo::RecordJointCreation(tj, this, nts);
    SALVAGER("End of Split()");
#endif
}


BOOL TrackSeg::SnapIntoLine (WP_cord& wpx1, WP_cord& wpy1) {
    /* convert to polar coordinates around END[0]*/
    double ptx = wpx1 - Ends[0].wpx;
    double pty = wpy1 - Ends[0].wpy;
#ifdef CRAZY_OLD_TRIGONOMETRIC_SnapIntoLine
    double phi = atan2 (pty, ptx);
    double theta = (SinTheta == 0.0f) ? 0.0 : atan2 (SinTheta, CosTheta);
    double psi = phi - theta;
    if (psi < -CONST_PI)
	psi += CONST_2PI;
    if (psi < -CONST_PI_OVER_2 || psi > CONST_PI_OVER_2)
	return FALSE;
    double ptlen = (pty == 0.0) ? fabs(ptx) : sqrt (ptx*ptx + pty*pty);
    double rely = ptlen * sin(psi);
    if (fabs(rely) > SNAP_DELTA)
	return FALSE;
    double relx = ptlen * cos(psi);
#else /* new way 29 June 1999 */
    /* see linedist.html */
    /* relx and rely have precisely the same meanings as above,
       i.e., relx = directed projection of the point-vector upon the trackseg
             rely = directed perpendicular from the point to the trackseg */
    double relx = CosTheta * ptx + SinTheta * pty;
    if (relx < 0.0)
	return FALSE;
    double rely = CosTheta * pty - SinTheta * ptx;
    if (fabs(rely) > SNAP_DELTA)
	return false;
#endif
    if (relx > Length)
	return FALSE;

    wpx1 = (int) (relx * CosTheta + Ends[0].wpx + .5);
    wpy1 = (int) (relx * SinTheta + Ends[0].wpy + .5);

    return TRUE;
}

static TrackSeg * S_ts;
static WP_cord S_wpx, S_wpy;

static int SnapMapper (GraphicObject *go) {
    TrackSeg * ts = (TrackSeg *) go;
    SC_cord scx = WPXtoSC(S_wpx), scy = WPYtoSC(S_wpy);
    
    if (ts->GraphicObject::HitP (scx, scy)) {
	if (ts->SnapIntoLine (S_wpx, S_wpy)) {
	    S_ts = ts;
	    return 1;
	}
    }
    return 0;
}

#ifdef REALLY_NXSYS
Virtual void TrackSeg::EditContextMenu(HMENU m) {
    if (OwningTurnout)
	OwningTurnout->EditContextMenu(m);
    else {
	if (Circuit->Occupied)
	    DeleteMenu(m, ID_OCCUPY, MF_BYCOMMAND);
	else
	    DeleteMenu(m, ID_VACATE, MF_BYCOMMAND);
	if (!TrainCount)
    	    EnableMenuItem(m, ID_TRAIN, MF_BYCOMMAND|MF_GRAYED);
    }
}


Virtual void TrackSeg::Hit (int mb) {
    if (OwningTurnout && (mb == WM_RBUTTONDOWN || mb == WM_NXGO_RBUTTONCONTROL))
	OwningTurnout->Hit(mb, this);
    else {
#ifdef NXSYSMac
        if (GlobalOfferingChooseTrack) {
            GlobalChooseTrackHandler(this);
            return;
        }
#endif
	if (mb == WM_LBUTTONDOWN && Circuit)
	    Circuit->SetOccupied (!Circuit->Occupied);
	else if (Circuit && mb == WM_NXGO_RBUTTONCONTROL) {
	    switch (RunContextMenu (IDR_TRACK_CONTEXT_MENU)) {
		case ID_TRAIN:
		    if(TrainCount)
			WindowToTopFromTrackUnit(this);
		    break;
		case ID_OCCUPY:
		    Circuit->SetOccupied (TRUE);
		    break;
		case ID_VACATE:
		    Circuit->SetOccupied (FALSE);
		    break;
		case ID_DRAW_RELAY:
		    DrawRelaysForObject ((int)Circuit->StationNo, "Track Section");
		    break;
		case ID_RELAY_QUERY:
		    ShowStateRelaysForObject ((int)Circuit->StationNo, "Track Section");
		    break;
	    }
	}
    }
}
#endif

Virtual BOOL TrackSeg::HitP (long x, long y) {
    if (!GraphicObject::HitP(x, y))
	return FALSE;
    WP_cord wpxx = (WP_cord)SCXtoWP((SC_cord)x);
    WP_cord wpyy = (WP_cord)SCYtoWP((SC_cord)y);

    if (SnapIntoLine (wpxx, wpyy)) {
	if (Ends[0].ExLight)
	    if (Ends[0].ExLight->HitP (x, y))
		return FALSE;
	if (Ends[1].ExLight)
	    if (Ends[1].ExLight->HitP (x, y))
		return FALSE;
	return TRUE;
    }
    return FALSE;
}


TrackSeg * SnapToTrackSeg (WP_cord& wpx, WP_cord& wpy) {
    S_ts = NULL;
    S_wpx = wpx;
    S_wpy = wpy;
    if (MapGraphicObjectsOfType  (TypeId::TRACKSEG, SnapMapper)) {
	wpx = S_wpx;
	wpy = S_wpy;
	return S_ts;
    }
    return NULL;
}

static inline bool coord_vaguely_valid(int x) {
    return x >= 0 && x <= 20000;
}

static bool tj_vaguely_valid(TrackJoint* tj) {
    /* avoid crashes when deleting ... this is still a dicey proposition */
    /* this SHOULD never fail, but ... part of anti-bug quest */
    if (!(
             coord_vaguely_valid ((int) tj->wp_x)
          && coord_vaguely_valid ((int) tj->wp_y)
#ifdef TLEDIT
          && coord_vaguely_valid ((int) tj->rw_x)
          && coord_vaguely_valid ((int) tj->rw_y)
#endif
    ))
        return false;

    if (tj->TSCount < 0 || tj->TSCount > 3) return false;
    if (tj->SwitchAB0 < 0 || tj->SwitchAB0 > 2) return false;
    if (tj->Nomenclature < 0 || tj->Nomenclature > 99999) return false;
    return true;
}

TrackSeg::~TrackSeg () {
    for (int i = 0; i < 2; i++) {
	TrackJoint * tj = Ends[i].Joint;
        if (tj) {
            if (!tj_vaguely_valid(tj)) {
#ifdef DEBUG
                assert(!"Invalid joint pointer in track seg destructor.");
#endif
                return; // otherwise, just don't do it.
            }
	    tj->DelBranch(this);
	    if (tj->TSCount == 0)
		if (!NXGODeleteAll)
		    delete tj;
	}
    }
    if (Circuit)
	Circuit->DeleteSeg(this);
}

TypeId TrackSeg::TypeID () { return TypeId::TRACKSEG;};


bool TrackSeg::IsNomenclature(long x) {
    return false;
}


#ifdef REALLY_NXSYS 
int TrackSeg::StationPointsEnd (WP_cord &wpcordlen, TSEX end_index, int loop_check) {

    if (loop_check >= 200) {
        MessageBox(0, "Chasing infinite track loop.", "Train system init", 0);
        wpcordlen = 0; /* satisfy flow analyzer, which really checks recursive calls! */
        return 0;
    }
    //TrackSegEnd * local = &Ends[end_index];
    TSEX tse = (end_index == TSEX::E0) ? TSEX::E1 : TSEX::E0;
    TrackSegEnd * distant = &GetEnd(tse);
    int sno;
    wpcordlen = (int) Length;
    if (distant->Joint) {
	TrackJoint * tj = distant->Joint;
	if (tj->Insulated && (sno = tj->StationNumber()))
	    return sno;
    }
    WP_cord farwplen;
    sno = distant->Next->StationPointsEnd
	       (farwplen, distant->EndIndexNormal, loop_check + 1);
    wpcordlen += farwplen;
    return sno;
}


void TrackSeg::SpreadRWFactor (double factor) {
    if (RWLength >= 0.0)
	return;
    RWLength = (float) (Length*factor);
    Ends[0].SpreadRWFactor(factor);
    Ends[1].SpreadRWFactor(factor);
}

void TrackSegEnd::SpreadRWFactor (double factor) {
    if (Joint && Joint->Insulated)
	return;
    if (Next)
	Next->SpreadRWFactor(factor);
    if (NextIfSwitchThrown)
	NextIfSwitchThrown->SpreadRWFactor(factor);
}

void TrackSeg::UpdateCircuitOccupation(){
    if (Circuit)
        Circuit->ComputeOccupiedFromTrains();
}

void TrackSeg::IncrementTrainOccupation() {
    TrainCount += 1;
    UpdateCircuitOccupation();
}

void TrackSeg::DecrementTrainOccupation(){
    assert (TrainCount);
    TrainCount -= 1;
    UpdateCircuitOccupation();
}
#endif
