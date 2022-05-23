#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "math.h"
#include <unordered_set>
#include <cassert>

#include <vector>   // Vectorized for global array and local segs 9/27/2019

#if TLEDIT
#include "undo.h"
#endif

#ifdef REALLY_NXSYS
#include "relays.h"
#include "trkload.h"
#include "xturnout.h"
#include "rlyapi.h"
#endif

/* Track circuits are not GraphicObjects and need explicit tracking.
   Track seg(ment)s, on the other hand, are GO's and are tracked by the
   GO system.
 */

static std::vector<TrackCircuit*> AllTrackCircuits;

TrackCircuit::TrackCircuit (IJID sno) {
    assert(sno && "Attempt to create track circuit 0");
    Occupied = Routed = Coding = FALSE;
    TrackRelay = NULL;
    StationNo = sno;
};

/* One can imagine that an std::set might be better if this is frequent,
 but it doesn't happen in real NXSYS, and in TLEdit is infrequent.*/
template <class T>
static void VEDEL (std::vector<T> v, T item) {  //1767-1808 Ukr. vector deleter
    for (auto it = v.begin(); it != v.end(); it++) {
        if (*it == item) {
            v.erase(it);
            break;
        }
    }
} 

TrackCircuit::~TrackCircuit () {
    VEDEL(AllTrackCircuits, this);
    for (auto ts : Segments)
	ts->Circuit = NULL;
}


static TrackCircuit* CreateNewTrackCircuit (IJID sno) {
    TrackCircuit* new_circuit = new TrackCircuit(sno);
    AllTrackCircuits.push_back(new_circuit);
    return new_circuit;
}

TrackCircuit* FindTrackCircuit (IJID sno) {
    for (TrackCircuit * tc : AllTrackCircuits)
	if (tc->StationNo == sno)
            return tc;
    return NULL;
}

TrackCircuit* GetTrackCircuit (IJID sno) {
    assert(sno && "GetTrackCircuit called on 0");
    TrackCircuit * tc = FindTrackCircuit (sno);
    return tc ? tc : CreateNewTrackCircuit(sno);
}

void TrackCircuit::DeleteSeg (TrackSeg * ts) {
        VEDEL(Segments, ts);
}

void TrackCircuit::AddSeg (TrackSeg * ts) {
    ts->Circuit = this;
    for (auto ats : Segments)  // shouldn't really be in array
        if (ats == ts)
            return;
    Segments.push_back(ts);
}

TrackCircuit * TrackSeg::SetTrackCircuit (IJID tcid) {
    if (tcid == 0) {
        if (Circuit) {
            Circuit->DeleteSeg(this);
            Circuit = NULL;
            Invalidate();
        }
        return NULL;
    }
    TrackCircuit * tc = GetTrackCircuit (tcid);
    SetTrackCircuit0 (tc);
    Invalidate();
    return tc;
}

void TrackSeg::SetTrackCircuit0 (TrackCircuit * tc) {
    if (Circuit != tc) {
	if (Circuit)
	    Circuit->DeleteSeg(this);
	if (tc)
	    tc->AddSeg(this);
        else
            Circuit = nullptr;
        Invalidate();
        
    }
}

#ifdef TLEDIT

void TrackSeg::SetTrackCircuitWildfire(IJID tcid) {

    TrackCircuit* tc = tcid ? GetTrackCircuit(tcid) : NULL;

    SegmentGroupMap SGM;
    
    CollectContacteesRecurse(SGM);

    for (auto [seg, cct] : SGM)
        seg->SetTrackCircuit0(tc);

    Undo::RecordWildfireTCSpread(SGM, tcid);

    if (tc)
        tc->SetRouted(tcid != 0);
};

void TrackSeg::CollectContacteesRecurse (SegmentGroupMap& SGM) {
    /* this is all that keeps us from infinite recursion */
    if (SGM.count(this))
        return;

    SGM[this] = TCNO();
    
    for (int ex = 0; ex < 2; ex++) {
        TrackSegEnd* ep = &Ends[ex];
        TrackJoint* tj = ep->Joint;
        assert(tj);
        if (!tj->Insulated) {
            for (int i = 0; i < tj->TSCount; i++) {
                if (tj->TSA[i] != this)
                    tj->TSA[i]->CollectContacteesRecurse(SGM);
            }
        }
    }
}

std::pair<int,int> TrackSeg::AnalyzeSegmentGroup(const SegmentGroupMap &M){
    std::unordered_set<IJID> circuits;
    int unassigned = 0;
    for (auto [seg, tcid] : M)
        if (tcid == 0)
            unassigned ++;
        else
            circuits.insert(tcid);
    return {circuits.size(), unassigned};
}



#endif

void TrackCircuit::SetOccupied (BOOL sta, BOOL force) {
    if ((Occupied != sta) || force) {
	Occupied = sta;
	Invalidate();
#ifdef REALLY_NXSYS
	if (TrackRelay)
	    ReportToRelay (TrackRelay, !Occupied);
#endif
    }
}

void TrackCircuit::SetRouted (BOOL sta) {
    if (Routed != sta) {
	Routed = sta;
	Invalidate();
    }
}

void TrackCircuit::Invalidate() {
    for (auto ts : Segments) {
#ifdef REALLY_NXSYS
	if (ts->Ends[0].Joint && ts->Ends[0].Joint->TurnOut)
	    ts->Ends[0].Joint->TurnOut->InvalidateAndTurnouts();
	if (ts->Ends[1].Joint && ts->Ends[1].Joint->TurnOut)
	    ts->Ends[1].Joint->TurnOut->InvalidateAndTurnouts();
#endif
	ts->Invalidate();
    }
}

#ifdef REALLY_NXSYS
void TrackCircuit::ProcessLoadComplete () {
    TrackRelay = CreateAndSetReporter (StationNo, "T", TrackReportFcn, this);
    CreateAndSetReporter (StationNo, "K", TrackKRptFcn, this);
}


void ReportAllTrackSecsClear () {
    for (auto tc : AllTrackCircuits)
	tc->SetOccupied(FALSE, TRUE); //force=
}


void ClearAllTrackSecs () {
    for (auto tc : AllTrackCircuits)
	tc->SetOccupied(FALSE);
}

void TrackCircuitSystemLoadTimeComplete () {
    for (auto tc : AllTrackCircuits)
	tc->ProcessLoadComplete();
}

void TrackCircuitSystemReInit() {
    std::unordered_set<TrackCircuit*> Circuits;
    for (auto tc : AllTrackCircuits)
        Circuits.insert(tc);
    for (auto ttc : Circuits)
        delete ttc;
    AllTrackCircuits.clear();
}

void TrackCircuit::TrackReportFcn(BOOL state, void* v) {
    TrackCircuit * t = (TrackCircuit *) v;
    t->Occupied = !state;
    t->Invalidate();
}

void TrackCircuit::TrackKRptFcn(BOOL state, void* v) {
    ((TrackCircuit *) v)->SetRouted (state);
}


void TrackCircuit::ComputeSwitchRoutedState () {
    /* route all segments that have both ends clear to an IJ with
       no trailed trailing point switches */

    for (auto ts : Segments)
	ts->ComputeSwitchRoutedState ();
}

void TrackSeg::ComputeSwitchRoutedState () {
    BOOL oldstate = Routed;
    Routed = ComputeSwitchRoutedEndState(0)
	     && ComputeSwitchRoutedEndState(1);
    if (oldstate != Routed)
	Invalidate();
}

BOOL TrackSeg::ComputeSwitchRoutedEndState(int ex) {
    TrackSegEnd * ep = &Ends[ex];
    if (ep->InsulatedP())
	return TRUE;
    if (ep->Next == NULL)
	return TRUE;
    TrackJoint * tj = ep->Joint;
    if (tj == NULL) {
next:
	return ep->Next->ComputeSwitchRoutedEndState(1-(int)ep->EndIndexNormal);
    }

    if (tj->TSCount < 3)
	return TRUE;
    if (tj->TurnOut == NULL)
	return TRUE;
    BOOL Thrown = tj->TurnOut->Thrown;
    if (this == (*tj)[TSAX::REVERSE])
	if (Thrown)
	    goto next;
	else
	    return FALSE;
    else if (this == (*tj)[TSAX::NORMAL])
	if (Thrown)
	    return FALSE;
	else
	    goto next;
    else {
	if (Thrown)
	    return ep->NextIfSwitchThrown
		    ->ComputeSwitchRoutedEndState(1-(int)ep->EndIndexReverse);
	else
	    goto next;
    }
}

GraphicObject * FindDemoHitCircuit (long id) {
    TrackCircuit * tc = FindTrackCircuit (id);
    if (tc == NULL)
	return NULL;
    return tc->FindDemoHitSeg();
}

TrackSeg * TrackCircuit::FindDemoHitSeg() {
    for (auto ts : Segments) {
	if (!ts->OwningTurnout)
	    return ts;
    }
    return Segments[0];
}

void TrackCircuit::ComputeOccupiedFromTrains () {
    int occ = 0;
    for (auto ts : Segments)
	occ += ts->TrainCount;
    SetOccupied ((BOOL)(occ != 0));
}


#endif


BOOL TrackCircuit::MultipleSegmentsP() {
    return Segments.size() > 1;
}
