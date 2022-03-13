#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "xtgtrack.h"
#include "tletoolb.h"
#ifndef NXSYSMac
#include "rubberbd.h"
#endif
#include "objid.h"
#include "tledit.h"
#include "resource.h"
#include "assignid.h"
#include "signal.h"

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

    DefStartTrackJoint = (TrackJoint *)FindHitObjectOfType(ID_JOINT, x, y);
    DefCreatedTrackJoint = (DefStartTrackJoint == NULL);
    DefStartTrackSeg = NULL;
    DefStartDelay = FALSE;

    if (DefCreatedTrackJoint) {
	WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);
	DefStartTrackSeg = SnapToTrackSeg (wpx, wpy);
	if (DefStartTrackSeg) {
	    DefStartTrackSeg->Select();
	    DefStartWPX = (int) wpx;
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


static void DefButtonUp (int x, int y) {
    
    WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);

    TrackJoint * tj = (TrackJoint *) FindHitObjectOfType (ID_JOINT, x, y);
    TrackSeg * ts = NULL;

    if (!DefStartTrackJoint || (tj && tj->AvailablePorts() == 0))
	goto clr;

    if (tj == NULL) {
	ts = SnapToTrackSeg (wpx, wpy);
	tj = new TrackJoint (wpx, wpy);
    }
    else {
	wpx = tj->wp_x;
	wpy = tj->wp_y;
	if (tj == DefStartTrackJoint) {
	    if (DefCreatedTrackJoint)
		delete tj;
#ifndef NXSYSMac  /* this screws up on Mac because of left's redef */
	    StatusMessage ("  ");
#endif   /* this is a kludge of ignorance */
	    goto clr;
	}
    }

    BufferModified = TRUE;

    if (ts)
	ts->Split (wpx, wpy, tj);

    if (DefStartTrackSeg)
	DefStartTrackSeg->Split (DefStartTrackJoint->wp_x,
				 DefStartTrackJoint->wp_y,
				 DefStartTrackJoint);

    if (tj) {
	TrackSeg * tsg = new TrackSeg
			 (DefStartTrackJoint->wp_x,
			  DefStartTrackJoint->wp_y, wpx, wpy);

	tsg->Ends[0].Joint = DefStartTrackJoint;
	tsg->Ends[1].Joint = tj;
	tj->AddBranch(tsg);
	tj->EnsureID();
	DefStartTrackJoint->AddBranch(tsg);
	DefStartTrackJoint->EnsureID();
	tsg->Select();
    }

clr:
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
    MovTrackJoint = (TrackJoint *)FindHitObjectOfType(ID_JOINT, x, y);
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
	int fx = MovTrackJoint->FindEndIndex(ts);
        if (fx != TSA_NOTFOUND) {
            MovRB[j] = new RubberBand (hWnd,
				       WPXtoSC(ts->Ends[1-fx].wpx),
                                       WPYtoSC(ts->Ends[1-fx].wpy),
                                       j
                                       );
        }
    }
}

static void MovButtonUp (HWND hWnd, int x, int y) {

    MoveModeCollineate(x, y);

    WP_cord wpx = SCXtoWP(x), wpy = SCYtoWP(y);

    TrackJoint * tj = (TrackJoint *)FindHitObjectOfType(ID_JOINT, x, y);

    if (tj && tj != MovTrackJoint) {	/* Try to merge nodes */
	if (tj->TSCount + MovTrackJoint->TSCount > 3)
	    return;
	wpx = tj->wp_x;
	wpy = tj->wp_y;
	MovTrackJoint->SwallowOtherJoint (tj);
    }

    BufferModified = TRUE;
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
		BufferModified = TRUE;
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

    TrackSeg * ts1 = TSA[0];
    TrackSeg * ts2 = TSA[1];
    int fx[2];
    fx[0] = FindEndIndex(ts1);
    fx[1] = FindEndIndex(ts2);
    if (fx[0] == TSA_NOTFOUND || fx[1] == TSA_NOTFOUND) {
	usererr ("BUG: tracks and joint estranged; won't delete");
	return;
    }
    for (int jj = 0; jj < 2; jj++) {
	TrackSegEnd * epp = &TSA[jj]->Ends[fx[jj]];
	if (epp->SignalProtectingEntrance) {
	    epp->SignalProtectingEntrance->PSignal->Select();
	    usererr ("Cannot delete a joint with signals at it. "
		     "Delete this signal first.");
	    return;
	}
	if (epp->ExLight) {
	    epp->ExLight->Select();
	    usererr ("Cannot delete a joint with exit lights at it. "
		     "Delete this exit light first.");
	    return;
	}
    }

    ts1->Invalidate();
    ts2->Invalidate();
    TrackSegEnd * ep = &ts1->Ends[fx[0]];
    *ep = ts2->Ends[1-fx[1]];
    if (ep->SignalProtectingEntrance) {
	PanelSignal * ps = ep->SignalProtectingEntrance->PSignal;
	ps->Seg = ts1;
	ps->Reposition();
    }
    ep->Joint->DelBranch(ts2);
    ep->Joint->AddBranch(ts1);
    ts1->Align();
    ts1->ComputeVisibleLast();		/* s/b in align? */
    ts1->Select();
    ts1->Invalidate();
    delete ts2;
    delete this;
    BufferModified = TRUE;
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
    BufferModified = TRUE;
}



void TrackJoint::MoveToNewWPpos (WP_cord wpx1, WP_cord wpy1) {

    MoveSC (WPXtoSC(wpx1), WPYtoSC(wpy1));

    for (int j = 0; j < TSCount; j++) {
	TrackSeg * ts = TSA[j];
	int fx = FindEndIndex(ts);
	if (fx != TSA_NOTFOUND) {
	    ts->Hide();
	    ts->Ends[fx].wpx = wpx1;
	    ts->Ends[fx].wpy = wpy1;
	    ts->Align(); //ts->Ends[0].wpx, was ist das?
	    ts->MakeSelfVisible();
	}
    }
    PositionLabel();
}


void TrackJoint::SwallowOtherJoint (TrackJoint * tj) {
    for (int j = 0; j < tj->TSCount; j++) {
	TrackSeg * ts = tj->TSA[j];
	AddBranch(ts);
        if (TSCount > 2) {
	    EnsureID();
            Insulated = FALSE; // 3-12-2022
        }
	int fx = tj->FindEndIndex(ts);
	if (fx != TSA_NOTFOUND) 
	    ts->Ends[fx].Joint = this;
    }
    delete tj;
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
		       Seg->Ends[EndIndex].Joint->Nomenclature);
    else
	StatusMessage ("Auto signal at IJ %ld",
		       Seg->Ends[EndIndex].Joint->Nomenclature);
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
	BufferModified = TRUE;
    }
    PositionLabel();
}

void TrackJoint::FlipNum() {
    if (TSCount > 1) {
	NumFlip = !NumFlip;
	BufferModified = TRUE;
	PositionLabel();
    }
}

void TrackJoint::Insulate() {
    if (!Insulated) {
	Insulated = TRUE;
	EnsureID();
	BufferModified = TRUE;
	Invalidate();
    }
}

int TrackJoint::SignalCount () {
    int s = 0;
    for (int i = 0; i < TSCount; i++) {
	TrackSeg * ts = TSA[i];
	int ex = FindEndIndex(ts);
	if (ts->Ends[ex].SignalProtectingEntrance)
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
    BufferModified = TRUE;		/* to put it mildly */
}


Virtual void	GraphicObject:: Cut() {
    BufferModified = TRUE;
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
