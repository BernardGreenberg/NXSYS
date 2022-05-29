#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cassert>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "lyglobal.h"
#include "typeid.h"
#include "brushpen.h"
#include "SwitchConsistency.h"
#include "salvager.hpp"

#include "pival.h"

#ifdef TLEDIT
#include "assignid.h"
#include "tledit.h"
#define SNAP_DELTA (2*Track_Width_Delta)
#else
#include <signal.h>
#include "rlyapi.h"
#define SNAP_DELTA Track_Width_Delta

#endif

BOOL ShowNonselectedJoints = TRUE;

static int JointGlyphRadius;

TrackJoint::TrackJoint (WP_cord wpx1, WP_cord wpy1) {
    Nomenclature = 0L;
    SwitchAB0 = 0;
#if NXSYSMac
    JointGlyphRadius = (int)(1.5*GU2);
#else
    JointGlyphRadius = (int) (2*GU2);
#endif
    Insulated = FALSE;
    Lab = NULL;
    TSCount = 0;
    TSA[0] = TSA[1] = TSA[2] = NULL;
    NumFlip = FALSE;
    wp_x = wpx1;
    wp_y = wpy1;

#if TLEDIT
    Organized = FALSE;
    Selected = FALSE;
    Marked = FALSE;
    EditAsJointInProgress = false;
    rw_x = rw_y = 0;
    int r = 2*JointGlyphRadius;
    wp_limits.left  = -r;
    wp_limits.right = +r;
    wp_limits.top   = -r;
    wp_limits.bottom= +r;
    MakeSelfVisible();
#else
    TurnOut = NULL;
#endif
}


TrackJoint::~TrackJoint () {
#if TLEDIT
    if (Nomenclature != 0)
	DeAssignID ((int)Nomenclature);
#endif
    if (!NXGODeleteAll)
	delete Lab;
}


void TrackJoint::Display (HDC dc) {
#if TLEDIT
    if (Selected)
	SelectObject (dc, GKGreenBrush);
    else if (!ShowNonselectedJoints)
	return;
    else if (Insulated)
	SelectObject (dc, GKYellowBrush);
    else 
	SelectObject (dc, GKRedBrush);
    SelectObject (dc, GetStockObject (NULL_PEN));

    Ellipse (dc,
	     sc_x - JointGlyphRadius, sc_y - JointGlyphRadius,
	     sc_x + JointGlyphRadius, sc_y + JointGlyphRadius);
    
#endif
}

TrackSeg* TrackJoint::GetBranch(TSAX branch_index) {
    if (branch_index == TSAX::STEM || branch_index == TSAX::IJR0)
        return TSA[0];
    else if (branch_index == TSAX::NORMAL || branch_index == TSAX::IJR1)
        return TSA[1];
    else if (branch_index == TSAX::REVERSE)
        return TSA[2];
    else
        assert(!"Invalid Branch Index");
    return nullptr;
}

BOOL TrackJoint::AddBranch (TrackSeg * ts) {
    if (AvailablePorts() <= 0)
	return FALSE;
    TSA[TSCount++] = ts;
    return TRUE;
}

TSAX TrackJoint::FindBranchIndex (TrackSeg * ts) {
    if (ts == NULL)
        return TSAX::NOTFOUND;
    for (int i = 0; i < TSCount; i++)
	if (TSA[i] == ts)
	    for (int j = 0; j < 2; j++)
		if (ts->Ends[j].Joint == this)
		    return (TSAX)j;
    return TSAX::NOTFOUND;
}

int TrackJoint::AvailablePorts () {
    if (Insulated && TSCount == 2)  // Do not create "insulated switches" - cannot be repr in LAYOUT
        return 0;   // 3-17-2022
    return 3-TSCount;
}

/* this is utterly useless when swapping segments going into a switch, as it hoses the nomenclature and the label.  See new TrackJoint::Cut. */

void TrackJoint::DelBranch (TrackSeg * ts) {
    for (int i = 0; i < TSCount; i++)
	if (TSA[i] == ts) {
#ifdef TLEDIT
            if (TSCount == 3) {
                SwitchConsistencyUndefine(Nomenclature, SwitchAB0);
                SwitchAB0 = 0;
                Nomenclature = AssignID(1);
                PositionLabel();
            }
#endif
/* In new "Limbo" regime, nulling these pointers is a bad idea.  They can be resurrected.
    Final destruction (layout unload/shutdown) does not come through here. This can only be
    the result of editing or Redo */
#if 0
            if (ts->Ends[0].Joint == this)
                ts->Ends[0].Joint = nullptr;
            if (ts->Ends[1].Joint == this)
                ts->Ends[1].Joint = nullptr;
#endif
            TSCount--;

	    for (int j = i; j < TSCount; j++)
		TSA[j] = TSA[j+1];
	    break;
	}
    for (int i = TSCount; i < 3; i++)  //Clear unused slots for better look in debugger
        TSA[i] = nullptr;
}

TypeId TrackJoint::TypeID () {return TypeId::JOINT;}
bool TrackJoint::IsNomenclature(IJID x) {return Nomenclature == x;}



void TrackJoint::PositionLabel() {

    char buf[10];
    JointOrganizationData jod[3];
    if (Lab && (Nomenclature > 10000)) {
        Lab->SetText("");
        Lab->Invalidate();
        return;
    }

    if (Nomenclature == 0 || Nomenclature >= 10000)
	return;

    if (Lab == NULL)
#ifdef TLEDIT
	Lab = new NXGOLabel (this, wp_x, wp_y, "foo");
#else
	Lab = new NXGOLabel (NULL, wp_x, wp_y, "foo");
#endif
    else (Lab->Hide());

    sprintf (buf, "%ld", (IJID)Nomenclature);
    if (TSCount == 3 && SwitchAB0 != 0) 
	strcat (buf, (SwitchAB0 == 1) ? "A" : "B");
    Lab->SetText (buf); 

    GetOrganization(jod);
    /* pointer to biggest interangle */
    JointOrganizationData * jdl = &jod[TSCount-1];
    if (TSCount != 1)
	if (jdl->SignalExists ^ NumFlip)
	jdl--;

    int foo = 0;
    if (jod[0].SignalExists && jod[1].SignalExists) {
        jdl++;
        foo++;
    }

    double angle = jdl->Radang + jdl->Interang/2;
    if (TSCount == 1)
	angle = jdl->Radang + CONST_PI*1.5;
    float rad = Lab->Radius();
    if (foo)
	rad /= 2;
    double fxc = wp_x + rad*cos(angle);
    double fyc = wp_y + rad*sin(angle);
    Lab->PositionCenter ((WP_cord) fxc, (WP_cord) fyc);
    Lab->MakeSelfVisible();
}


double CircleSub (double x, double y) {
    double result = x - y;
    return (result < 0.0) ? (result + 2.0*CONST_PI) : result;
}


static int OrgDataCompare (const void * e1p, const void * e2p) {
    /* smallest angle (stem) will come out first, biggest (rev) last */
    JointOrganizationData * s1 = (JointOrganizationData *) e1p;
    JointOrganizationData * s2 = (JointOrganizationData *) e2p;
    if (s1->Interang == s2->Interang)
	return 0;
    if (s1->Interang < s2->Interang)
	return -1;
    return +1;
}

void TrackJoint::GetOrganization (JointOrganizationData *jod) {
    int i;
//    assert(TSCount == 3); not so
    /* Compute positive, clockwise angles from positive X origin */
    for (i = 0; i < TSCount; i++) {
	TrackSeg * ts = TSA[i];
	jod[i].original_index = i;	/* for degubbing.` */
	jod[i].TSeg = ts;
        if (ts == nullptr)
            return;
        TSEX endx = ts->FindEndIndex(this);
#if TLEDIT
	if (endx == TSEX::NOTFOUND) {
	    usererr ("GetJointOrganization finds estranged segment/joint.");
	    return;
	}
#endif
        TrackSegEnd *farp = &ts->GetOtherEnd(endx);
	jod[i].SignalExists =(ts->GetEnd(endx).SignalProtectingEntrance != NULL);
	jod[i].Radang = CircleSub (atan2 ((double)(farp->wpy - wp_y),
					 (double)(farp->wpx - wp_x)), 0.0);
    }

    /* Compute Positive, clockwise angles from TSA[0] */
	   
    double r0 = jod[0].Radang;
    for (i = 0; i < TSCount; i++)
	jod[i].RRadang = CircleSub (jod[i].Radang, r0);

    /* Get #1 & #2 in clockwise order from 0 - we have + angles off 0,
       so it is now easy. */
    if (TSCount > 2)
	if (jod[1].RRadang > jod[2].RRadang)
            std::swap(jod[1], jod[2]);

    /* NOW compute clockwise differences */

    for (i = 0; i < TSCount; i++) {
	jod[i].Interang = CircleSub(jod[(i+1) % TSCount].RRadang,
				    jod[i].RRadang);
	/* Set the ID of the tseg opposing this angle */
	jod[i].OpposingSeg = jod[(i+TSCount-1) % TSCount].TSeg;
    }

    /* C'est magi-que !*/
    qsort (jod, TSCount, sizeof (jod[0]), OrgDataCompare);

}

#if REALLY_NXSYS
void DecodeDigitated (long input, int &track_no, int &station_no) {

    if (input == 0)
        track_no = station_no = 0;
    else if (Glb.IRTStyle) {
	track_no    = input % 10;
	station_no  = (int)(input / 10);
    }
    else {
        int n_digits_minus_1 = (int)log10(input);
        int power = (int)pow(10, n_digits_minus_1);
        track_no   = (int)(input / power);
        station_no = input % power;
    }
}

int TrackJoint::StationNumber () {
    int track_no, station_no;
    DecodeDigitated (Nomenclature, track_no, station_no);
    return station_no;
}
#endif
