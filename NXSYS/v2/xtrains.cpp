#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cassert>

#include "xtgtrack.h"
#include "lyglobal.h"
#include "signal.h"

#include "traindcl.h"
#include "timers.h"
#include "compat32.h"
#include "traincfg.h"
#include "trainaut.h"
#include "objid.h"
#include "xturnout.h"

#include "nxproduct.h"

// SULT = "SetUpLayout - Train Metrics" */


static TSEX flip_end (TSEX endx) {
    if (endx == TSEX::E0)
        return TSEX::E1;
    else if (endx == TSEX::E1)
        return TSEX::E0;
    else
        assert (!"Bad end index to trains' flip_end");
    return TSEX::E0;
}

#if 0
static int SULTMMapper3 (GraphicObject * g) {
    TrackSeg* ts = (TrackSeg *) g;
    /* for debugging/development/default */
    ts->RWLength = (float) (ts->Length/50);
    return 0;
}
#endif

static int SULTMMapper1 (GraphicObject * g) {
    TSEX endx;

    TrackSeg* ts = (TrackSeg *) g;
    if (ts->RWLength >= 0.0f)		/* processed already. */
	return 0;

    if (ts->Ends[0].Joint && ts->Ends[0].Joint->Insulated
	&& ts->Ends[0].Joint->Nomenclature)
	endx = TSEX::E0;
    else if (ts->Ends[1].Joint && ts->Ends[1].Joint->Insulated
	     && ts->Ends[1].Joint->Nomenclature)
	endx = TSEX::E1;
    else return 0;
    WP_cord wpcordlen;
    int this_joint_sn = ts->GetEnd(endx).Joint->StationNumber();
    int that_joint_sn = ts->StationPointsEnd (wpcordlen, endx, 0);
    double rwlen = fabs((double)(this_joint_sn-that_joint_sn));
    ts->SpreadRWFactor(rwlen/wpcordlen);
    return 0;
}

static int SULTMMapper2 (GraphicObject * g) {
    TrackSeg* ts = (TrackSeg *) g;
    /* for debugging/development/default */
    if (ts->RWLength == -1.0f)
	ts->RWLength = (float) (ts->Length/50);
    return 0;
}


/* delay until first train ?  */
void SetUpLayoutTrainMetrics () {
    MapGraphicObjectsOfType (TypeId::TRACKSEG, SULTMMapper1);
    MapGraphicObjectsOfType (TypeId::TRACKSEG, SULTMMapper2);
}


Signal * Train::FindNextSig () {
    front.FindTrackSeg();
    X_Of_Next_Signal = front.x_at_seg_start;
    TrackSeg * ts = front.ts;
    TSEX endx = front.facing_ex;
    while (ts) {
        TrackSegEnd * ep = &ts->GetEnd(endx);
	X_Of_Next_Signal += ts->RWLength;
	if (ep->Next == NULL)
	    return NULL;
	if (ep->FacingSwitch == NULL || !ep->FacingSwitch->Thrown) {
	    ts = ep->Next;
            endx = flip_end(ep->EndIndexNormal);
	}
	else {
	    ts = ep->NextIfSwitchThrown;
            endx = flip_end(ep->EndIndexReverse);
	}
	Signal * s = ts->GetOtherEnd(endx).SignalProtectingEntrance;
	if (s)
	    return s;
    }
    return NULL;
}


BOOL VerifyTrackSelectionAcceptability (TrackUnit * ts) {
    if (ts->Ends[0].Next != NULL
	&& ts->Ends[1].Next != NULL) {
	    MessageBox (0, "Selected track section is not an end-of-track; one is required.",
			PRODUCT_NAME " train manager", MB_OK|MB_ICONEXCLAMATION);
	    return FALSE;
    }
    return TRUE;
}

void Train::InitPositionTracking (TrackUnit * ts) {
    /* really track seg */

    TSEX endx = TSEX::E0; /* undef val bothers mac analyzer, I suppose it's right ... */
    if (ts->Ends[0].Next == NULL)
	endx = TSEX::E1;
    else if (ts->Ends[1].Next == NULL)
	endx = TSEX::E0;
    front.ts = ts;
    front.x = 0.0;
    front.x_at_seg_start = 0.0;
    front.facing_ex = endx;
    front.IAmFront = TRUE;
    front.Trn = this;
    front.PassJoint (ts->GetOtherEnd(endx).Joint);
    SetOccupied(ts);

    /* now figure out the back of the train. */
    back = front;
    back.x = -Length;
    back.IAmFront = FALSE;
}

void Pointpos::PassJoint (TrackJoint * tj) {
    int tno, sno;
    DecodeDigitated (tj->Nomenclature, tno, sno);
    if (Glb.IRTStyle)
	sprintf (LastIJID, "%d/%s", sno*10+tno, Glb.RouteIdentifier.c_str());
    else
	sprintf (LastIJID, "%s%d-%d", Glb.RouteIdentifier.c_str(), tno, sno);
    FeetSinceLastIJ = FSLIJatSegStart = 0.0f;
}

void Pointpos::Reverse (double new_x) {
    if (ts) {
	strcpy (LastIJID, "");
	FSLIJatSegStart = FeetSinceLastIJ = 0.0f;

        facing_ex = flip_end(facing_ex);
	x_at_seg_start = new_x - (ts->RWLength - fabs(x - x_at_seg_start));
	x = new_x;
    }
}

int Pointpos::FindTrackSeg () {

    if (ts) {			/* train not moving off end of trk */
	double feet_left = x - x_at_seg_start;
	while (feet_left > 0.0) {
	    double segfeet = feet_left - ts->RWLength;
	    if (segfeet < 0.0) {
		FeetSinceLastIJ
			= (float)(FSLIJatSegStart + feet_left);
		return +1;
	    }
	    feet_left = segfeet;
	    /* must move into next track segment */
	    if (!IAmFront)
		Trn->SetUnoccupied (ts);
            TrackSegEnd * ep = &ts->GetEnd(facing_ex);

	    if (ep->Joint && ep->Joint->Insulated && ep->Joint->Nomenclature){
		PassJoint (ep->Joint);
		FeetSinceLastIJ = 0.0f;
	    }

	    FSLIJatSegStart = FeetSinceLastIJ;

	    if (ep->Next == NULL) {	/* going off track */
		FeetSinceLastIJ = (float)feet_left;
		return -1;
	    }
	    /* check for derail */

	    x_at_seg_start += ts->RWLength;

	    if (ep->FacingSwitch == NULL || !ep->FacingSwitch->Thrown) {
		ts = ep->Next;
                facing_ex = flip_end(ep->EndIndexNormal);
	    }
	    else {
		ts = ep->NextIfSwitchThrown;
                facing_ex = flip_end(ep->EndIndexReverse);
	    }

	    if (IAmFront)
		Trn->SetOccupied (ts);
	}
    }
    return +1;
}


int Train::ComputeOccupations() {
    
    front.FindTrackSeg();
    back.x = front.x - Length/100.0;
    if (back.x >= 0.0) {
	if (back.FindTrackSeg() < 0) {
	    /* train has run off map */
	    return 0;
	}
    }
    return 1;
}

static int FTETSBNMapper (GraphicObject * g, void * v) {
    TrackSeg * ts = (TrackSeg *) g;
    long id = *(long *) v;
    if (ts->Ends[0].Joint && ts->Ends[0].Joint->Nomenclature == id)
	return 1;
    if (ts->Ends[1].Joint && ts->Ends[1].Joint->Nomenclature == id)
	return 1;
    return 0;
}
    

TrackUnit *  FindTrainEntryTrackSectionByNomenclature (long id_no) {

    return (TrackUnit *)
	    MapFindGraphicObjectsOfType (TypeId::TRACKSEG, FTETSBNMapper, &id_no);
}
