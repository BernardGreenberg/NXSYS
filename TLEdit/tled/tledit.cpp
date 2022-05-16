#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <unordered_set>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "xtgtrack.h"
#include "tletoolb.h"
#ifndef NXSYSMac
#include "rubberbd.h"
#endif
#include "typeid.h"
#include "tledit.h"
#include "resource.h"
#include "assignid.h"
#include "signal.h"
#include "salvager.hpp"
#include "undo.h"

#ifdef NXSYSMac
void StartRubberBand(int index, long x, long y);
void RubberBandTo(int index, long x, long y);
void RubberBandToHighlighted(int index, long x, long y);
void RubberBandOff(int index);
#define WPXtoSC(x) ((int)(x))
#define WPYtoSC(y) ((int)(y))
#define SCXtoWP(x) ((int)(x))
#define SCYtoWP(y) ((int)(y))
struct RubberBand {
    int StartX, StartY, J;

    RubberBand(HWND, int x, int y, int j) {
        StartX = x;
        StartY = y;
        J = j;
        StartRubberBand(J, x, y);
    }
    ~RubberBand() {
        RubberBandOff(J);
    }
    void Draw(int x, int y) {
        RubberBandTo(J, x,y);
    }
    void DrawHighlighted(int x, int y) {
        RubberBandToHighlighted(J, x,y);
    }
};
#endif

#define TAN_SNAP_FACTOR 25

static TrackSeg * DefStartTrackSeg = NULL;
static TrackJoint* DefStartTrackJoint = NULL;
static BOOL        DefCreatedTrackJoint = FALSE;
static RubberBand * DefRB = NULL;

static BOOL DefStartDelay = FALSE;
static int DefStartWPX;
static int DefStartWPY;

static TrackJoint * MovTrackJoint = NULL;
static RubberBand * MovRB[3] = {NULL, NULL, NULL};

#ifdef NXSYSMac

#endif
static void ReportCoordsWP (WP_cord x, WP_cord y) {
    StatusMessage ("X = %d  Y = %d", (int) x, (int) y);
}

static void ReportCoordsSC (SC_cord x, SC_cord y) {
    ReportCoordsWP (SCXtoWP(x), SCYtoWP(y));
}

static BOOL DefButtonDown (int& x, int &y) {

    DefStartTrackJoint = (TrackJoint *)FindHitObjectOfType(TypeId::JOINT, x, y);
    DefCreatedTrackJoint = (DefStartTrackJoint == NULL);
    DefStartTrackSeg = NULL;
    DefStartDelay = FALSE;
    StatusMessage("");

    if (DefCreatedTrackJoint) {
	WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);
	DefStartTrackSeg = SnapToTrackSeg (wpx, wpy);
	if (DefStartTrackSeg) {
	    DefStartTrackSeg->Select();
	    DefStartWPX = (int)wpx;
	    DefStartWPY = (int)wpy;
	    DefStartTrackJoint = NULL;
	    DefStartDelay = TRUE;
	    return TRUE;
	}
	DefStartTrackJoint = new TrackJoint (wpx, wpy);
    }
    else {
	x = DefStartTrackJoint->sc_x;
	y = DefStartTrackJoint->sc_y;
    }
    ReportCoordsSC (x, y);

    if (DefStartTrackJoint) {
	DefStartTrackJoint->Select();
	if (DefStartTrackJoint->AvailablePorts() == 0) {
	    DefStartTrackJoint = NULL;
	    return FALSE;
	}
    }
    return TRUE;
}

static bool OneOfRaysOfStart (TrackJoint* radiator, TrackJoint* tj) {
    /* 3-17-2022 --  Disallow direct overlay of extant segment !!!! Causes chaos and loops!  */
    /* It would be cool to cancel if you get near. Collinear drops are a problem, too. */
    for (int i = 0; i < radiator->TSCount; i++) {
        TrackSeg* ray = radiator->TSA[i];
        TSEX myex = ray->FindEndIndex(radiator);  /* trick and track magliozzi */
        TrackSegEnd& E = ray->GetOtherEnd(myex);
        TrackJoint* tom = E.Joint;
        if (tj == tom)
            return true;
    }
    return false;
}

static bool DropOnRayFromStart(TrackSeg* ts) {
    for (int i = 0; i < DefStartTrackJoint->TSCount; i++) {
        if (ts == DefStartTrackJoint->TSA[i])
            return true;
    }
    return false;
}

static bool valid_drop_target(TrackJoint* tj){
    if (tj == DefStartTrackJoint)
        return false;
    if (tj->AvailablePorts() <= 0)
        return false;
    if (OneOfRaysOfStart(DefStartTrackJoint, tj))
        return false;
    
    return true;
}

static void DefButtonUp2 (int x, int y) {
    
    TrackJoint * tj = (TrackJoint *) FindHitObjectOfType (TypeId::JOINT, x, y);

    if (tj == NULL) {
        WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);
	TrackSeg* ts = SnapToTrackSeg (wpx, wpy);
        if (ts) {
            if (DropOnRayFromStart(ts)) {// 3-17-2022
                StatusMessage("Cannot drop on emanation from same joint");
                return;
            }
            tj = new TrackJoint(wpx, wpy);
            ts->Split (wpx, wpy, tj);
        }
        else
            tj = new TrackJoint(wpx, wpy);
    }
    else if (!valid_drop_target(tj)) {
        if (tj == DefStartTrackJoint)
            StatusMessage("");
        else
            StatusMessage ("Invalid joint for drop target");
        if (DefCreatedTrackJoint)
            delete DefStartTrackJoint;
        return;
    }
    /* else drop into new space or tj */

    /* This is the COMMIT point */
    Undo::RecordIrreversibleAct("create track segment");

    if (DefStartTrackSeg)
	DefStartTrackSeg->Split (DefStartTrackJoint->wp_x,
				 DefStartTrackJoint->wp_y,
				 DefStartTrackJoint);

    if (tj) {
	TrackSeg * ts = new TrackSeg (DefStartTrackJoint->wp_x,
                                      DefStartTrackJoint->wp_y,
                                      tj->wp_x, tj->wp_y);

	ts->Ends[0].Joint = DefStartTrackJoint;
	ts->Ends[1].Joint = tj;
	tj->AddBranch(ts);

	tj->EnsureID();
	DefStartTrackJoint->AddBranch(ts);
	DefStartTrackJoint->EnsureID();
	ts->Select();
        SALVAGER("After DefButtonUp2");
    }

}

static void DefButtonUp (int x, int y) {
    
    if (DefStartTrackJoint)
        DefButtonUp2 (x, y);
    
    DefStartTrackSeg = NULL;
    DefCreatedTrackJoint = FALSE;
    DefStartTrackJoint = NULL;
}

static BOOL Collineate (WP_cord wpx0, WP_cord wpy0,
			WP_cord wpx1, WP_cord wpy1,
			WP_cord wpxo, WP_cord wpyo,
			WP_cord &wpx, WP_cord &wpy) {


    WP_cord vecnum = wpy1 - wpy0;
    WP_cord vecden = wpx1 - wpx0;
    WP_cord bandnum = wpy - wpyo;
    WP_cord bandden = wpx - wpxo;

    WP_cord difnum = bandnum*vecden - vecnum*bandden;
    WP_cord difden = bandden*vecden + bandnum*vecnum;
    if (difden == 0)
	return FALSE;
    if (TAN_SNAP_FACTOR*fabs((float)difnum) > fabs((float)difden))
	return FALSE;

    double difang = atan2 ((double)difnum, (double)difden);
    double vecang = atan2 ((double)vecnum, (double)vecden);
    double bandhyp = sqrt ((double)(bandnum*bandnum + bandden*bandden));
    double projext = bandhyp*cos(difang);

    wpx = (WP_cord)(wpxo + projext*cos(vecang) + .5);
    wpy = (WP_cord)(wpyo + projext*sin(vecang) + .5);
    return TRUE;
}	

#if 0 // call is apparently "not necessary"
static BOOL CollineateJointSeg (TrackJoint * tj, TrackSeg * ts,
			       WP_cord &wpx, WP_cord &wpy) {
    int fx = tj->FindEndIndex (ts);
    if (fx == TSA_NOTFOUND)
	return FALSE;
    int otherfx = 1 - fx;
    return Collineate (ts->Ends[otherfx].wpx, ts->Ends[otherfx].wpy, 
		       ts->Ends[fx].wpx, ts->Ends[fx].wpy, 
		       tj->wp_x, tj->wp_y,
		       wpx, wpy);
#if 0
    TrackJoint * tj2 = ts->FindOtherJoint (tj);
    return Collineate (tj2->wp_x, tj2->wp_y,
		       tj->wp_x, tj->wp_y,
		       tj->wp_x, tj->wp_y,
		       wpx, wpy);
#endif
}
#endif




static int MoveModeCollineate (SC_cord &x, SC_cord &y) {
    WP_cord wpx = SCXtoWP (x), wpy = SCYtoWP (y);
    int j, tsc;
    tsc = MovTrackJoint->TSCount;

    /* Line up with all arcs going into other end of arcs radiating
       from this point */
    for (j = 0; j < tsc; j++) {
	TrackSeg * tsarc = MovTrackJoint->TSA[j];
	TrackJoint * tjdist = tsarc->FindOtherJoint(MovTrackJoint);
	int disttsc = tjdist->TSCount;
	for (int k = 0; k < disttsc; k++) {
	    TrackSeg * tsdistdist = tjdist->TSA[k];
	    TrackJoint * tjdistdist = tsdistdist->FindOtherJoint(tjdist);
	    if (Collineate (tjdist->wp_x, tjdist->wp_y,
			    tjdistdist->wp_x, tjdistdist->wp_y,
			    tjdist->wp_x, tjdist->wp_y,
			    wpx, wpy))
		goto store_cvt;
	}
    }
    
    /* Line up with straight lines between the distant nodes */
    if (tsc > 1)
	for (j = 0; j < tsc; j++) {
	    TrackSeg * tsarc = MovTrackJoint->TSA[j];
	    TrackJoint * tjdist = tsarc->FindOtherJoint(MovTrackJoint);
	    TrackSeg * tsarc2 = MovTrackJoint->TSA[(j+1) % tsc];
	    TrackJoint * tjdist2 = tsarc2->FindOtherJoint(MovTrackJoint);
	    if (Collineate (tjdist->wp_x, tjdist->wp_y,
			    tjdist2->wp_x, tjdist2->wp_y,
			    tjdist->wp_x, tjdist->wp_y,
			    wpx, wpy))
		goto store_cvt;
	}

    /* Line up with horizontal */
    for (j = 0; j < tsc; j++) {
	TrackSeg * tsarc = MovTrackJoint->TSA[j];
	TrackJoint * tjdist = tsarc->FindOtherJoint(MovTrackJoint);
	if (Collineate (0, 0, 100, 0,
			tjdist->wp_x, tjdist->wp_y,
			wpx, wpy))
		goto store_cvt;
    }

#if 0
    /* Line up with original arcs - perhaps this is not necessary ? */
    for (j = 0; j < tsc; j++) {
	TrackJoint * tj2
		= MovTrackJoint->TSA[j]->FindOtherJoint (MovTrackJoint);
	for (int k = 0; k < tj2->TSCount; k++)
	    if (CollineateJointSeg (MovTrackJoint, tj2->TSA[k], wpx, wpy))
		goto store_cvt;
    }	
#endif

    return -1;
store_cvt:
    x = WPXtoSC (wpx);
    y = WPYtoSC (wpy);
    return j;
}

static void MovButtonDown (HWND hWnd, int x, int y) {
    MovTrackJoint = (TrackJoint *)FindHitObjectOfType(TypeId::JOINT, x, y);
    WP_cord wpx = SCXtoWP (x), wpy = SCYtoWP(y);
    if (MovTrackJoint == NULL) {
	TrackSeg * ts = SnapToTrackSeg (wpx, wpy);
	if (ts) {
	    MovTrackJoint = new TrackJoint (wpx, wpy);
	    ts->Split (wpx, wpy, MovTrackJoint);
	}
	else
	    return;
    }
    MovTrackJoint->Select();
    ReportCoordsWP (MovTrackJoint->wp_x, MovTrackJoint->wp_y);
    for (int j = 0; j < MovTrackJoint->TSCount; j++) {
	TrackSeg * ts = MovTrackJoint->TSA[j];
	TSEX fx = ts->FindEndIndex(MovTrackJoint);
        if (fx != TSEX::NOTFOUND) {
            MovRB[j] = new RubberBand (hWnd,
				       WPXtoSC(ts->GetOtherEnd(fx).wpx),
                                       WPYtoSC(ts->GetOtherEnd(fx).wpy),
                                       j
                                       );
        }
    }
}

static std::unordered_set<TrackJoint*> all_far_nodes (TrackJoint* tj) {
    std::unordered_set<TrackJoint*> S;
    for (int i = 0; i < tj->TSCount; i++){
        TrackSeg * ts = tj->TSA[i];
        TrackJoint* other = ts->FindOtherJoint(tj);
        S.insert(other);
    }
    return S;
}

static bool set_intersects(std::unordered_set<TrackJoint*> s1, std::unordered_set<TrackJoint*> s2) {
    for (auto s1i = s1.begin(); s1i != s1.end(); s1i++)
        if (s2.count(*s1i))
            return true;
    return false;
}

static void MovButtonUp (HWND hWnd, int x, int y) {

    MoveModeCollineate(x, y);

    WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);

    TrackJoint * tj = (TrackJoint *)FindHitObjectOfType(TypeId::JOINT, x, y);

    if (!tj) {
        TrackSeg* ts = (TrackSeg*)FindHitObjectOfType(TypeId::TRACKSEG, x, y);
        if (ts && MovTrackJoint->FindBranchIndex(ts) == TSAX::NOTFOUND) {
            usererr("A joint being moved may not be directly dropped onto a track segment. "
                    "You can only drop into empty space or onto extant joints. "
                    "Create a new joint here if that is what you want, and then move.");
            return;
        }
    }
    assert(MovTrackJoint!=nullptr); /* not at all likely, but placates flow analyzer */
    
    if (tj == MovTrackJoint)
        tj = nullptr;  // fall into "just move" 3-26-2022
    
    if (tj) {	/* Try to merge nodes */
        if (OneOfRaysOfStart(MovTrackJoint, tj)) {// disallow drop on current neighbor node 3-18-2022
            StatusMessage("Cannot drop joint on end of ray from itself; cut segment instead.");
            return;
        }
        if (set_intersects(all_far_nodes(tj), all_far_nodes(MovTrackJoint))) {
            usererr("A joint being moved may not be dropped on a joint at the end of a colocated segment.  Delete the segment if that is what you want.");
            return;
        }

	if (tj->TSCount + MovTrackJoint->TSCount > 3)
	    return;
	wpx = tj->wp_x;
	wpy = tj->wp_y;
	MovTrackJoint->SwallowOtherJoint (tj);
    }
    else Undo::RecordIrreversibleAct("move or create track joint");

    MovTrackJoint->MoveToNewWPpos (wpx, wpy);
}


static void MoveModeRodentate (HWND hWnd, UINT message, int x, int y) {

static BOOL MovButtonFirst = TRUE;

    if (message != WM_RBUTTONDOWN)
	if (!MovTrackJoint)
	    return;

    switch (message) {
	case WM_RBUTTONUP:

#ifdef NXSYSMac
        case WM_LBUTTONUP:
#endif
	    if (!MovButtonFirst)
		MovButtonUp (hWnd, x , y);
	    else
                Undo::RecordIrreversibleAct("create mid-seg track joint");
	    MovTrackJoint = NULL;
	    for (int j = 0; j < 3; j++) {
#ifdef NXSYSMac
                RubberBandOff(j);
#endif
                delete MovRB[j];
                MovRB[j] = NULL;
	    }
	    break;
	case WM_RBUTTONDOWN:
        case WM_NXGO_LBUTTONCONTROL:
	    MovButtonFirst = TRUE;
	    MovButtonDown (hWnd, x, y);
	    break;
	case  WM_MOUSEMOVE:
	    if (MovTrackJoint) {
		MovButtonFirst = FALSE;
		int colx = MoveModeCollineate (x, y);
		ReportCoordsSC (x, y);
                for (int j = 0; j < 3; j++) {
		    if (MovRB[j]) {
			if (j == colx)
			    MovRB[j]->DrawHighlighted(x, y);
			else
			    MovRB[j]->Draw(x, y);
		    }
                }
	    }
	    break;
    }
    SALVAGER("After rodentate");

}

static void DefineModeRodentate (HWND hWnd, UINT message, int x, int y) {

    if (message != WM_LBUTTONDOWN)
	if (!DefRB)
	    return;
#ifdef NXSYSMac
    if (MovTrackJoint) {
        if (message == WM_LBUTTONDOWN || message == WM_NXGO_LBUTTONCONTROL) {
            return;
        }
    }
#endif

    if ((message == WM_MOUSEMOVE || message == WM_LBUTTONUP) && DefRB) {
	int delx = x - DefRB->StartX;
	int dely = y - DefRB->StartY;
	if (fabs((float)dely) < fabs((float)delx)/TAN_SNAP_FACTOR)
	    y = DefRB->StartY;
	if (fabs ((float)delx) < fabs((float)dely)/TAN_SNAP_FACTOR)
	    x = DefRB->StartX;
    }

    switch (message) {
	case WM_LBUTTONUP:
	    DefButtonUp (x, y);
	    delete DefRB;
            DefRB = NULL;
	    break;
	case WM_LBUTTONDOWN:
            if (DefButtonDown (x, y)) {
                DefRB = new RubberBand (hWnd, x, y, 0);
            }
             break;
	case WM_MOUSEMOVE:
	    if (DefStartDelay) {
		DefStartTrackJoint=new TrackJoint (DefStartWPX, DefStartWPY);
		DefStartTrackJoint->Select();
		DefStartDelay = FALSE;

                x = WPXtoSC(DefStartWPX);
		y = WPYtoSC(DefStartWPY);
	    }
	    ReportCoordsSC (x, y);
            DefRB->Draw (x, y);
            break;
    }

}

static int ctsmapper (GraphicObject * g, void * v) {
    POINT* p = (POINT*) v;
    return (g->ClickToSelectP() & g->HitP(p->x, p->y));
}

void TrackLayoutRodentate (HWND hWnd, UINT message, int x, int y) {
    GraphicObject * g;
    POINT P;
    P.x = x;
    P.y = y;
    switch (message) {
	case WM_LBUTTONDOWN:
	    g = MapAllVisibleGraphicObjects (ctsmapper, &P);
	    if (g)
		g->EditClick(x, y);
	    else
		DefineModeRodentate (hWnd, message, x, y);
	    return;
	case WM_LBUTTONUP:
#ifdef NXSYSMac
            if (MovTrackJoint) {
                MoveModeRodentate(hWnd, WM_RBUTTONUP, x, y);
            }
#endif
	    DefineModeRodentate (hWnd, message, x, y);
	    return;
	case WM_RBUTTONDOWN:
#ifdef NXSYSMac
            g = GetMouseHitObject(x, y);
#else
	    g = MapAllVisibleGraphicObjects (ctsmapper, &P);
#endif
            if (g) {
		g->Select();
		g->EditProperties();
		return;
	    }
#ifdef NXSYSMac
            break; // don't fall through, use control key
        case WM_NXGO_LBUTTONCONTROL:
            MoveModeRodentate (hWnd, WM_RBUTTONDOWN, x, y);
            break;
#endif
	case WM_RBUTTONUP:

            MoveModeRodentate (hWnd, message, x, y);
	    return;
	case WM_MOUSEMOVE:
	    if (DefRB)
		DefineModeRodentate(hWnd, message, x, y);
	    else if (MovTrackJoint)
		MoveModeRodentate(hWnd, message, x, y);
	    return;
    }
}


void InsulateJoint (TrackJoint * tj) {
    if (tj->TSCount != 1 && tj->TSCount != 2) {
	usererr ("Only inline or terminal joints may be insulated.");
	return;
    }
    if (!tj->Insulated) {
	tj->Insulate();
	StatusMessage ("IJ %ld is now insulated.", tj->Nomenclature);
    }
}

static bool triangle_collapse_condition(TrackJoint * tj){
    assert(tj->TSCount == 2);  // only valid call
    /* If this node and the two nodes at the other ends of its two rays
     form a triangle, disallow the deletion of this node, which would
     create a situation of two overlaid rays between the same two nodes */

    TSEX myx0 = tj->TSA[0]->FindEndIndex(tj);
    if (myx0 == TSEX::NOTFOUND)
        return false;  // should not happen;
    TrackJoint * tj_other_0 = tj->TSA[0]->GetOtherEnd(myx0).Joint;

    TSEX myx1 = tj->TSA[1]->FindEndIndex(tj);
    if (myx1 == TSEX::NOTFOUND)
        return false;  // should not happen;
    TrackJoint * tj_other_1 = tj->TSA[1]->GetOtherEnd(myx1).Joint;
    
    for (int i = 0; i < tj_other_0->TSCount; i++) {
        TrackSeg * tstest = tj_other_0->TSA[i];
        for (int j = 0; j < tj_other_1->TSCount; j++) {
            if (tj_other_1->TSA[j] == tstest)
                return true;
        }
    }
    return false;
}


void TrackJoint::Cut () {
    switch (TSCount) {
	case 0:
	    delete this;
	    return;
	case 1:
	    usererr ("Cannot delete a track termination joint. "
		     "Delete the segment emanating from it.");
	    return;
	case 2:
	    break;
	case 3:
	    usererr ("Cannot delete a switch joint. "
		     "Delete one branch first.");
	    return;
    }

    /* Guaranteed to be 2-branch joint*/
    if (TSA[0] == NULL || TSA[1]== NULL) {
        usererr ("BUG: Null segment pointer in alleged 2-branch IJ.");
        return;
    }

    TrackSeg& ts0 = *TSA[0];
    TrackSeg& ts1 = *TSA[1];

    TSEX endxs[2];
    /* These are the indexes in the end-arrays of our rays for this joint. */
    endxs[0] = ts0.FindEndIndex(this);
    endxs[1] = ts1.FindEndIndex(this);
    if (endxs[0] == TSEX::NOTFOUND || endxs[1] == TSEX::NOTFOUND) {
        usererr ("BUG: tracks and joint estranged; won't delete");
        return;
    }
    for (int jj = 0; jj < 2; jj++) {
        TrackSegEnd& E = TSA[jj]->GetEnd(endxs[jj]);
	if (E.SignalProtectingEntrance) {
	    E.SignalProtectingEntrance->PSignal->Select();
	    usererr ("Cannot delete a joint with signals at it. "
		     "Delete this signal first.");
	    return;
	}
	if (E.ExLight) {
	    E.ExLight->Select();
	    usererr ("Cannot delete a joint with exit lights at it. "
		     "Delete this exit light first.");
	    return;
	}
    }
    
    if (TSCount == 2 && triangle_collapse_condition(this)) {
        usererr("Triangle collapse error: Deleting this joint would create coincident "
                "segments (track collision), physically impossible. "
                "Delete the branches emanating from it if that is what you mean.");
        return;
    }

    ts0.Invalidate();
    ts1.Invalidate();
    /*
                    ts0          this         ts1
     O  ------------------------- O --------------------- O
        DETS0             OurETS0    OurETS1         DETS1
     changing to
                     ts0
     O  ------------------------------------------------- O
        DETS0                                      OurETS0
     former DETS1

    */
    
    /* ts0 is going to swallow ts1, which latter will be deleted */
    TrackSegEnd& OurEndInTS0 = ts0.GetEnd(endxs[0]);
    TrackSegEnd& DistantEndInTS1 = ts1.GetOtherEnd(endxs[1]);
    TrackSegEnd& OurEndInTS1 = ts1.GetEnd(endxs[1]);
    OurEndInTS0 = DistantEndInTS1;  // copy the data to make it so.

    if (DistantEndInTS1.SignalProtectingEntrance) {
	PanelSignal& S = *DistantEndInTS1.SignalProtectingEntrance->PSignal;
	S.Seg = &ts0;
	S.Reposition();
    }

    TrackJoint& DJ0 = *OurEndInTS0.Joint;
    /* DelBranch/AddBranch is useless for this -- loses nomenclature in between. 3/23/2022*/
    bool found = false;
    for (int i = 0; i < DJ0.TSCount; i++) {
        if (DJ0.TSA[i] == &ts1) {
            DJ0.TSA[i] = &ts0;
            found = true;
            break;
        }
    }
    assert(found);
    OurEndInTS1.Joint = nullptr;
    ts0.Align();
    ts0.ComputeVisibleLast();		/* s/b in align? */
    ts0.Select();
    ts0.Invalidate();
    // Salvager will fail if we salvage here.  ... Message box will make redisplay crash, too
    delete &ts1;
    delete this;
    SALVAGER("TrackJoint::Cut final");
    Undo::RecordIrreversibleAct("delete track joint");
}


TrackSeg* FindOtherSegOfTwo (TrackJoint * tj, TrackSeg * ts) {
    if (ts == tj->TSA[0])
	return tj->TSA[1];
    else if (ts == tj->TSA[1])
	return tj->TSA[0];
    else return NULL;
}

void TrackSeg::Cut () {
    for (int jj = 0; jj < 2; jj++) {
	TrackSegEnd * ep = &Ends[jj];
	if (ep->SignalProtectingEntrance) {
	    ep->SignalProtectingEntrance->PSignal->Select();
	    usererr ("Cannot delete a segment with signals looking at it. "
		     "Delete this signal first.");
	    return;
	}
	if (ep->ExLight) {
	    ep->ExLight->Select();
	    usererr ("Cannot delete a segment with exit lights on it. "
		     "Delete this exit light first.");
	    return;
	}
    }

    TrackJoint * j0 = Ends[0].Joint;
    TrackJoint * j1 = Ends[1].Joint;
    if (j0->TSCount == 2)
	FindOtherSegOfTwo (j0, this)->Select();
    else if (j1->TSCount == 2)
	FindOtherSegOfTwo (j1, this)->Select();
    else if (j0->TSCount == 3)
	j0->Select();
    else if (j1->TSCount == 3)
	j1->Select();
    delete this;
    SALVAGER("TrackSeg::Cut final");
    Undo::RecordIrreversibleAct("delete track segment");
}



void TrackJoint::MoveToNewWPpos (WP_cord wpx1, WP_cord wpy1) {

    MoveSC (WPXtoSC(wpx1), WPYtoSC(wpy1));

    for (int j = 0; j < TSCount; j++) {
	TrackSeg * ts = TSA[j];
	TSEX endx = ts->FindEndIndex(this);
	if (endx != TSEX::NOTFOUND) {
            TrackSegEnd &E = ts->GetEnd(endx);
	    E.wpx = wpx1;
	    E.wpy = wpy1;
            ts->Hide();
	    ts->Align(); //ts->Ends[0].wpx, was ist das?
	    ts->MakeSelfVisible();
	}
    }
    PositionLabel();
}


void TrackJoint::SwallowOtherJoint (TrackJoint * other_tj) {
    for (int j = 0; j < other_tj->TSCount; j++) {
        TrackSeg& S = *(other_tj->TSA[j]);
        TSEX his_end_in_this_ts = S.FindEndIndex(other_tj);
        assert(his_end_in_this_ts != TSEX::NOTFOUND);
        Insulated = FALSE; //otherwisem AddBranch will fail silently....
	AddBranch(&S);
        S.GetEnd(his_end_in_this_ts).Joint = this;
        
        if (TSCount > 2) {  // français pour "3"
	    EnsureID();
            Insulated = FALSE; // 3-12-2022
            Organized = FALSE; // 3-14-2022
        }
    }
    delete other_tj;
    Undo::RecordIrreversibleAct("merge track joints");
}

void TrackJoint::Organize () {

    if (Organized)
	return;
    JointOrganizationData jod[3];
    GetOrganization (jod);
    
    for (int i = 0; i < TSCount; i++)
	TSA[i] = jod[i].OpposingSeg;
    Organized = TRUE;
}


TrackJoint * TrackSeg::FindOtherJoint (TrackJoint *tj) {
    if (tj == Ends[0].Joint)
	return Ends[1].Joint;
    else if (tj == Ends[1].Joint)
	return Ends[0].Joint;
    else return NULL;
}



void PanelSignal::Select() {
    GraphicObject::Select();
    if (Sig->XlkgNo)
	StatusMessage ("Signal %d at IJ %ld",
		       Sig->XlkgNo, 
		       Seg->GetEnd(EndIndex).Joint->Nomenclature);
    else
	StatusMessage ("Auto signal at IJ %ld",
		       Seg->GetEnd(EndIndex).Joint->Nomenclature);
}

void TrackSeg::Select () {
    GraphicObject::Select();
    SelectMsg();
}

void TrackSeg::SelectMsg() {
    char circuit_description [40];

    WP_cord delty = Ends[1].wpy - Ends[0].wpy;
    WP_cord deltx = Ends[1].wpx - Ends[0].wpx;
    double angle = atan2((double)delty, (double)deltx) * 180.0 / CONST_PI;
    if (angle < -90.0)
	angle += 180.0;
    else if (angle > 90.0)
	angle -= 180.00;

    if (Circuit == NULL)
	strcpy (circuit_description, "(no TC assigned)");
    else 
	sprintf (circuit_description,
		  "TC %ld", Circuit->StationNo);
    StatusMessage ("Segment: (%d,%d)->(%d,%d), length %.1f direction %.1f  %s",
		   Ends[0].wpx, Ends[0].wpy,
		   Ends[1].wpx, Ends[1].wpy,
		   Length, -angle,
		   circuit_description);
}

void TrackJoint::Select () {
    GraphicObject::Select();
    if (TSCount==3)
	StatusMessage ("Switch %ld%c,   X = %ld, Y = %ld",
		       Nomenclature, " AB"[SwitchAB0], wp_x, wp_y);
    else
	if (!Insulated)
	    StatusMessage ("Track Vertex, X = %ld, Y = %ld", wp_x, wp_y);
	else if (Nomenclature != 0)
	    StatusMessage ("IJ %ld,  X = %ld, Y = %ld", Nomenclature,
			   wp_x, wp_y);
	else
	    StatusMessage ("Anonymous IJ,  X = %ld, Y = %ld", wp_x, wp_y);

}

void TrackJoint::EnsureID() {
    if (Nomenclature == 0) {
	Nomenclature = AssignID(1);
        Undo::RecordIrreversibleAct("assign track joint ID");
    }
    PositionLabel();
}

void TrackJoint::FlipNum() {
    if (TSCount > 1) {
	NumFlip = !NumFlip;
        Undo::RecordIrreversibleAct("flip track joint label");
	PositionLabel();
    }
}

void TrackJoint::Insulate() {
    if (!Insulated) {
	Insulated = TRUE;
	EnsureID();
        Undo::RecordIrreversibleAct("insulate track joint");
	Invalidate();
    }
}

int TrackJoint::SignalCount () {
    int s = 0;
    for (int i = 0; i < TSCount; i++) {
	TrackSeg * ts = TSA[i];
        TSEX endx = ts->FindEndIndex(this);
	if (ts->GetEnd(endx).SignalProtectingEntrance)
	    s++;
    }
    return s;
}

BOOL TrackSegEnd::InsulatedP() {
    return Joint->Insulated;
}


struct _sm_xy {
    int x, y;
};

int ShiftMapper1 (GraphicObject * g, void * v) {
    struct _sm_xy * xyp = (struct _sm_xy *) v;
    g->ShiftLayout (xyp->x, xyp->y);
    return 0;
}

int ShiftMapper2 (GraphicObject * g, void *) {
    g->ShiftLayout2 ();
    return 0;
}   


void ShiftLayout (int x, int y) {
    _sm_xy xy = {x, y};
    HCURSOR old_cursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
    MapAllGraphicObjects (ShiftMapper1, &xy);
    MapAllGraphicObjects (ShiftMapper2, &xy);
    SetCursor (old_cursor);
    ComputeVisibleObjectsLast();
    Undo::RecordIrreversibleAct("shift layout");
}


Virtual void	GraphicObject:: Cut() {
    Undo::RecordGOCut(this);
    delete this;
    StatusMessage ("  ");
}

Virtual void    GraphicObject::EditClick (int x, int y) {
    Select();
}


Virtual void    GraphicObject::ShiftLayout (int delta_x, int delta_y) {
    MoveWP (wp_x + delta_x, wp_y + delta_y);
}

Virtual void    GraphicObject::ShiftLayout2() {
}

Virtual void    TrackJoint::ShiftLayout2 () {
    MoveToNewWPpos (wp_x, wp_y);
}

Virtual BOOL    GraphicObject::ClickToSelectP() {return TRUE;}
Virtual BOOL    TrackJoint::ClickToSelectP() {
    return FALSE;
}
Virtual BOOL    TrackSeg::ClickToSelectP() {
    return FALSE;
}
