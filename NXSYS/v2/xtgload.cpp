#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <vector>
#include <string>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "swkey.h"
#include "signal.h"
#include "trafficlever.h"
#include "plight.h"
#include "pswitch.h"
#include "objid.h"
#include "lisp.h"
#include "xtgload.h"
#include "pival.h"
#include "SwitchConsistency.h"
#include "STLExtensions.h"



#ifdef TLEDIT

#else
#include "xturnout.h"
#include <signal.h>

static std::vector<TrackJoint*> AllJoints;
#endif

#ifdef NXSYSMac
#include "AppDelegateGlobals.h"  //used to tell Mac global layout dims
#endif

void XTGLoadClose() {
}

aDEFLSYM (LAYOUT);
aDEFLSYM (SWITCH);
aDEFLSYM (IJ);
aDEFLSYM (PATH);
aDEFLSYM (SIGNAL);
aDEFLSYM (STEM);
aDEFLSYM (NORMAL);
aDEFLSYM (REVERSE);
aDEFLSYM (A);
aDEFLSYM (B);
aDEFLSYM (L);
aDEFLSYM (T);
aDEFLSYM (R);
aDEFLSYM (NUMFLIP);
aDEFLSYM (NOSTOP);
aDEFLSYM (TC);
aDEFLSYM (EXITLIGHT);
aDEFLSYM (PLATENO);
aDEFLSYM (ID);
aDEFLSYM (SWITCHKEY);
aDEFLSYM (TRAFFICLEVER);
aDEFLSYM (PANELLIGHT);
aDEFLSYM (PANELSWITCH);
aDEFLSYM (TEXT);
DEFLSYM2(aVIEW_ORIGIN,VIEW-ORIGIN);

static std::vector<TrackJoint*> SwitchJoints;

static bool DecodeBranchType (Sexpr s, TSAX* brtype) {
    if (s == aSTEM)
	*brtype = TSAX::STEM;
    else if (s == aNORMAL)
	*brtype = TSAX::NORMAL;
    else if (s == aREVERSE)
	*brtype = TSAX::REVERSE;
    else return false;
    return true;
}

void XTGLoadInit() {
}


static TrackJoint * FindSwitchJoint (long id, int ab0) {
    for (auto tj : SwitchJoints)
	if (tj->Nomenclature == id && tj->SwitchAB0 == ab0)
	    return tj;
    return nullptr;
}

int ProcessLayoutForm (Sexpr s),
    ProcessTextForm (Sexpr s);

static int
    ProcessPathForm (Sexpr s),
    ProcessSignalForm (Sexpr s),
    ProcessExitlightForm (Sexpr s),
    ProcessSwitchkeyForm (Sexpr s),
    ProcessTrafficleverForm (Sexpr s),
    ProcessPanelLightForm (Sexpr s),
    ProcessPanelSwitchForm (Sexpr s),
    ProcessViewOriginForm (Sexpr s);

void InitXTGReader () {  // called from rdtrkcmn, too.
    SwitchJoints.clear();
    SwitchConsistencyClear();
#if  ! TLEDIT
    AllJoints.clear();
#endif
}

int XTGLoad (FILE * f) {

    Sexpr s = read_sexp (f);
    if (s.type != Lisp::tCONS || CAR(s)!= aLAYOUT) {
        LispBarf ("LAYOUT Form missing in file.");
        return 0;
    }
    InitXTGReader();
    return ProcessLayoutForm (CDR(s));
}

int ProcessLayoutForm (Sexpr f) {
    for ( ;f.type == Lisp::tCONS; f = CDR(f)) {
	Sexpr s = CAR(f);
#if 0
	MessageBox (0, s.PRep(), "Layout subform about to be processed", MB_OK);
#endif
	if (s.type != Lisp::tCONS) {
	    LispBarf ("Non-list element found in LAYOUT", s);
	    return 0;
	}
	Sexpr fn = CAR(s);
	Sexpr data = CDR(s);
	if (fn == aPATH) {
	    if (!ProcessPathForm (data))
		return 0;
	}
	else if (fn == aSIGNAL) {
	    if (!ProcessSignalForm (data))
		return 0;
	}
	else if (fn == aEXITLIGHT) {
	    if (!ProcessExitlightForm (data))
		return 0;
	}
	else if (fn == aVIEW_ORIGIN) {
	    if (!ProcessViewOriginForm (data))
		return 0;
	}
#ifndef NOAUXK
	else if (fn == aSWITCHKEY) {
	    if (!ProcessSwitchkeyForm (data))
		return 0;
	}
#endif
	else if (fn == aTRAFFICLEVER) {
	    if (!ProcessTrafficleverForm (data))
		return 0;
	}
	else if (fn == aPANELLIGHT) {
	    if (!ProcessPanelLightForm (data))
		return 0;
	}
	else if (fn == aPANELSWITCH) {
	    if (!ProcessPanelSwitchForm (data))
		return 0;
	}
	else if (fn == aTEXT) {
	    if (!ProcessTextForm (data))
		return 0;
	}
	else {
	    LispBarf ("Element other than PATH, SIGNAL, EXITLIGHT, SWITCHKEY, TEXT, or VIEW-ORIGIN found in LAYOUT ", s);
	    return 0;
	}
    }

    for (auto tj : SwitchJoints)
        if (auto r = SwitchConsistencyDefine(tj->Nomenclature, tj->SwitchAB0)) {
            LispBarf(r.value.c_str());
            return 0;
        }
#ifndef TLEDIT

    for (auto j : AllJoints) {  // 3-17-2022
        TrackJoint& J = *j;
        for (int i = 0; i < J.TSCount; i++)
            if (J.TSA[i] == nullptr) {
                std::string msg =
                FormatString("Corrupt layout. "
                             "Joint/switch #%ld (%d) claims %d branches, but TSA[%ld] is null. "
                             "Editing and and re-save in TLEdit may or may not help.",
                             J.Nomenclature, J.SwitchAB0, J.TSCount, i);
                LispBarf(msg.c_str());
                return 0;
            }
    }



    // DON'T demand matching in TLEdit, or you couldn't fix the problems.
    if (auto r = SwitchConsistencyTotalCheck()) {
        LispBarf(r.value.c_str());
        return 0;
    }
        
    
    for (auto j : AllJoints)
	j->PositionLabel();
    if (!SwitchJoints.empty())
        if (!CreateTurnouts(&SwitchJoints[0], (int)SwitchJoints.size()))
            return 0;
#endif
    
#if NXSYSMac
    RECT lims = NXGO_ComputeTrueLayoutDimensions();
    MacAssertTrueLayoutDims(lims.right-lims.left, lims.bottom-lims.top);
#endif
    return 1;
}

struct ppf_coords {
    WP_cord x;
    WP_cord y;
    WP_cord last_y;
    WP_cord last_x;
    BOOL flip;
    BOOL Collect (Sexpr s, Sexpr f);
};


BOOL ppf_coords::Collect (Sexpr s, Sexpr f) {
    flip = FALSE;
    if (s.type != Lisp::tCONS) {
	LispBarf ("Non-list where coordinate subform expected:", f);
	return FALSE;
    }
    if (CAR(s).type != Lisp::NUM) {
	LispBarf ("Bogus X-coordinate in form", f);
	return FALSE;
    }
    x = CAR(s);
    s = CDR(s);
    if (s.type == Lisp::tCONS) {
	if (CAR(s).type != Lisp::NUM) {
	    LispBarf ("Bogus Y-coordinate in switch", f);
	    return FALSE;
	}
	y = CAR(s);
    }
    else
	y = last_y;

    s = CDR(s);

    if (s.type == Lisp::tCONS) {
	if (CAR(s) == aNUMFLIP)
	    flip = TRUE;
    }
    return TRUE;
}

static void SetTCID (TrackSeg * ts, long id) {
    if (id)
#if TLEDIT
	ts->SetTrackCircuit(id, FALSE)->Routed = TRUE;
#else
        ts->SetTrackCircuit(id, FALSE);
#endif
}

static TrackSeg * LinkTS (TrackSeg * last_ts, TrackSeg* ts) {
#ifdef REALLY_NXSYS
    if (last_ts) {
	last_ts->Ends[1].EndIndexNormal = TSEX::E0;
	last_ts->Ends[1].Next = ts;
    }
    ts->Ends[0].Next = last_ts;
    ts->Ends[0].EndIndexNormal = TSEX::E1;
#endif
    return ts;
}


static int ProcessPathForm (Sexpr f) {
    TrackJoint * last_joint = NULL;
    BOOL first = TRUE, last_was_switch = FALSE;
    TSAX last_switch_arc_type = TSAX::NOTFOUND;
    struct ppf_coords coords {};;
    coords.last_y = 0; /* placate flow-analyzer */
    TrackSeg * ts;
    TrackSeg * last_ts = NULL;
    TrackJoint * tj;
    long last_tcid = 0;
    for (; f.type == Lisp::tCONS; f = CDR(f)) {
	Sexpr s = CAR(f);
	Sexpr whole_path_element = s;
	long Nomen = 0;
	BOOL insulated = FALSE;
	Sexpr key;
	if (s.type == Lisp::NUM) {
	    coords.x = (WP_cord) s;
	    coords.y = coords.last_y;
	    insulated = FALSE;
create_simple:
#if TLEDIT
	    tj = new TrackJoint (coords.x, coords.y);
#else
	    tj = insulated ? new TrackJoint (coords.x, coords.y) : NULL;
#endif
	    if (tj) {
		tj->NumFlip = coords.flip;
		tj->Insulated = insulated;
		tj->Nomenclature = Nomen;
	    }
	    if (!first) {
		ts = new TrackSeg (coords.last_x, coords.last_y,
				   coords.x, coords.y);
		last_ts = LinkTS (last_ts, ts);
                last_ts->Consume();
		SetTCID (ts, last_tcid);

		if (tj)
		    tj->AddBranch(ts);
		ts->Ends[0].Joint = last_joint;
		ts->Ends[1].Joint = tj;

		if (last_was_switch)
		    last_joint->TSA[(int)last_switch_arc_type] = ts;
		else
		    if (last_joint)
			last_joint->AddBranch (ts);
	    }
#if REALLY_NXSYS
	    if (tj)
		AllJoints.push_back(tj);
#endif
	    last_was_switch = FALSE;
	    last_joint = tj;
finish_create_any:
	    first = FALSE;
	    coords.last_x = coords.x;
	    coords.last_y = coords.y;
	    
	    continue;
	}
        if (last_joint)
            last_joint->Consume(); /* LispBarf's cause flow analyzer to flag last_joint leakage */
	if (s.type != Lisp::tCONS) {
	    LispBarf ("Element other than number or list in PATH", s);
	    return 0;
	}
	key = CAR(s);
	if (key.type == Lisp::NUM) {
	    coords.x = key;
	    if (CDR(s).type == Lisp::tCONS) {
		if (CADR(s).type != Lisp::NUM) {
		    LispBarf ("Coordinate-pair form in PATH not a pair of numbers.", s);
		    return 0;
		}
	    }
	    coords.y = CADR(s);
	    insulated = FALSE;
	    goto create_simple;
	}
	    
	if (key.type != Lisp::ATOM) {
	    LispBarf ("Form with non atomic head in PATH ", key);
	    return 0;
	}
	s = CDR(s);
	if (key == aIJ) {
	    if (ListLen(s) < 2) {
		LispBarf ("Not enough data in IJ description ", s);
		return 0;
	    }
	    insulated = TRUE;
	    Nomen = CAR(s);
	    if (!coords.Collect (CDR(s), whole_path_element))
		return 0;
	    goto create_simple;
	}
	else if (key == aTC) {
	    if (ListLen(s) < 1) {
		LispBarf ("Not enough data in TC description ", whole_path_element);
		return 0;
	    }
	    last_tcid = CAR(s);
	    continue;
	}
	else if (key != aSWITCH) {
	    LispBarf ("Unrecognized subform in PATH ", whole_path_element);
	    return 0;
	}

	/* (SWITCH) BRANCHTYPE swID x y */
	if (s.type != Lisp::tCONS) {
invsw:	    LispBarf ("Invalid SWITCH subform", s);
	    return 0;
	}
	if (CAR(s).type != Lisp::ATOM)
	    goto invsw;
	TSAX brtype;
	if (!DecodeBranchType (CAR(s), &brtype)) {
	    LispBarf ("Unknown switch branch type", CAR(s));
	    return 0;
	}
	if (s.type != Lisp::tCONS) {
	    LispBarf ("Missing nomenclature in switch", whole_path_element);
	    return 0;
	}

	s = CDR(s);

	if (CAR(s).type != Lisp::NUM) {
	    LispBarf ("Bogus Nomenclature ID switch", whole_path_element);
	    return 0;
	}
	Nomen = CAR(s);
	s = CDR(s);
	
	int ab0;
	Sexpr e = CAR(s);

	if (e.type == Lisp::NUM) {
	    if (e.u.n != 0) {
                std::string swdesc = FormatString("Switch %ld: bad numeric Switch A/B/0 tag:", Nomen);
                LispBarf (swdesc.c_str(), e);
                return 0;
	    }
	    ab0 = 0;
	}
	else if (e == aA)
	    ab0 = 1;
	else if (e == aB)
	    ab0 = 2;
	else {
            std::string swdesc = FormatString("Switch %ld: unknown numeric Switch A/B/0 tag:", Nomen);
            LispBarf (swdesc.c_str(), e);
            return 0;
        }
	s = CDR(s);

	BOOL have_coords = FALSE;

	if (s.type == Lisp::tCONS) {
	    if (!coords.Collect (s, whole_path_element))
		return 0;
	    have_coords = TRUE;
	}

	TrackJoint * swj = FindSwitchJoint (Nomen,ab0);
	if (swj != NULL) {
	    if (!(first || CDR(f) == NIL)) {
		LispBarf ("Previously-defined switch neither first nor last in path", whole_path_element);
		return 0;
	    }

	}
	if (swj == NULL) {
	    if (!have_coords) {
		LispBarf ("Switch Undefined: point coordinates missing", whole_path_element);
		return 0;
	    }
	    swj = new TrackJoint (coords.x, coords.y);
#if REALLY_NXSYS
	    AllJoints.push_back(swj);
#endif
#if TLEDIT
	    swj->Organized = TRUE;
#endif
	    swj->Nomenclature = Nomen;
	    swj->SwitchAB0 = ab0;
            SwitchJoints.push_back(swj);
	    swj->TSCount = 3;
	    swj->TSA[0] = swj->TSA[1] = swj->TSA[2] = NULL;
	    swj->NumFlip = coords.flip;
	}
	else {
	    coords.x = swj->wp_x;
	    coords.y = swj->wp_y;
	}

	if (first)
	    last_switch_arc_type = brtype;
	else {
	    ts = new TrackSeg (coords.last_x, coords.last_y,
			       coords.x, coords.y);
	    last_ts = LinkTS (last_ts, ts);
	    SetTCID (ts, last_tcid);
	    swj->TSA[(int)brtype] = ts;
	    ts->Ends[1].Joint = swj;
	    ts->Ends[0].Joint = last_joint;
#ifndef xTLEDIT
	    if (last_was_switch)
		last_joint->TSA[(int)last_switch_arc_type] = ts;
	    else
		if (last_joint)
		    last_joint->AddBranch (ts);
#endif
	    last_switch_arc_type
		    = (brtype == TSAX::STEM) ? TSAX::NORMAL : TSAX::STEM;
	}
/* NEED WHOLE BUSINESS FOR LINKING THESE JOINTS WITHOUT TJ NODES */
	last_joint = swj;
        last_joint->Consume();
	last_was_switch = TRUE;
	goto finish_create_any;
    }
    if (last_joint)
        last_joint->Consume();
    if (last_ts)
        last_ts->Consume();
    return 1;
}

static int SigMatchOrientation (char orient, TrackSeg * ts, int end_index) {

    double angle = atan2 (ts->SinTheta, ts->CosTheta);
    if (end_index)
	angle += CONST_PI;
    if (angle > CONST_2PI)
	angle -= CONST_2PI;

    if (angle < 0.0)
	angle += CONST_2PI;
    if (islower (orient))
	orient = toupper(orient);
    if ((angle < CONST_PI_OVER_2 || angle > CONST_3PI_OVER_2) && orient == 'R')
	return 1;
    if ((angle < CONST_PI && angle > 0.0) && orient == 'B')
	return 1;
    if ((angle > CONST_PI_OVER_2 && angle < CONST_3PI_OVER_2) && orient == 'L')
	return 1;
    if (angle > CONST_PI && orient == 'T')
	return 1;
    return 0;
}
    
static TrackSeg * FindBranchFromOrientation (TrackJoint * tj, char orient, TSEX& endx) {
    for (int i = 0; i < tj->TSCount; i++) {
	TrackSeg * ts = tj->TSA[i];
        endx = ts->FindEndIndex(tj);
	//TrackSegEnd * ep = &ts->Ends[ex];
	if (tj->TSCount == 1 || SigMatchOrientation (orient, ts, (int)endx))
	    return ts;
    }
    return NULL;
}


static TrackJoint * FindTrackJoint (long nn) {
#if TLEDIT
    return (TrackJoint *) FindHitObject (nn, ID_JOINT);
#else
    for (auto j : AllJoints)
	if (j->Nomenclature == nn)
	    return j;
    return NULL;
#endif
}


static int ProcessExitlightForm (Sexpr s) {
    if (s.type != Lisp::tCONS) {
	LispBarf ("EXITLIGHT form missing data.");
	return 0;
    }
    if (CAR(s).type != Lisp::NUM) {
	LispBarf ("EXITLIGHT form missing lever number.");
	return 0;
    }
    long xno = CAR(s);
    SPop(s);
    if (s == NIL) {
	PanelSignal * ps = (PanelSignal*) FindHitObject (xno, ID_SIGNAL);
	if (!ps) {
	    LispBarf ("Cannot find signal for EXITLIGHT", Sexpr(xno));
	    return 0;
	}
	ps->Seg->GetEnd(ps->EndIndex).ExLight
		= new ExitLight (ps->Seg, ps->EndIndex, (int)xno);
	return 1;
    }
    /* (xno orient ij) */
    Sexpr e = CAR(s);
    char orient;
    if (e.type == Lisp::ATOM) {
	if (e != aL && e != aR && e != aT && e != aB) {
	    LispBarf ("Unrecognized orientation symbol in EXITLIGHT",
		      Sexpr(xno), e);
	    return 0;
	}
	orient = e.u.s[0];
	SPop(s);
	long nn = CAR(s);
	TrackJoint * tj = FindTrackJoint (nn);
	if (!tj) {
	    LispBarf ("Cannot find Joint for EXITLIGHT", CAR(s));
	    return 0;
	}
	TSEX endx;
	TrackSeg * ts = FindBranchFromOrientation (tj, orient, endx);
	if (!ts) {
	    LispBarf ("Cannot find EXITLIGHT IJ/orient reference.");
	    return 0;
	}
	ts->GetEnd(endx).ExLight = new ExitLight (ts, endx, (int)xno);
	return 1;
    }
    LispBarf ("Bogus EXITLIGHT item", s);
    return 0;
}

static int ProcessSignalForm (Sexpr s) {
    Sexpr ss = s;
    long ijid = 0;
    Sexpr Heads = NIL;
    int xlno = 0;
    char orient = ' ';
    BOOL HasStop = TRUE;

    if (CAR(s).type != Lisp::NUM) {
	LispBarf ("IJ id number missing in SIGNAL form", ss);
	return 0;
    }
    ijid = CAR(s);
    SPop(s);
    if (CAR(s).type == Lisp::NUM) {
        xlno = CAR(s);  // conv opr
	SPop(s);
    }
    Sexpr e = CAR(s);
    if (e.type == Lisp::ATOM) {
	if (e != aL && e != aR && e != aT && e != aB) {
	    LispBarf ("Unrecognized orientation symbol in SIGNAL",
		      e, ss);
	    return 0;
	}
	orient = e.u.s[0];
	SPop(s);
    }
    if (CAR(s).type != Lisp::tCONS) {
	LispBarf("Missing heads list in SIGNAL", ss);
	return 0;
    }
    Heads = CAR(s);
    long StaNo = 0L;
    TrackJoint * tj = FindTrackJoint (ijid);
    if (tj == NULL) {
	LispBarf ("Cannot find Insulated Joint", ss);
	return 0;
    }
    while (s.type == Lisp::tCONS) {
	SPop(s);
	if (s.type == Lisp::tCONS && CAR(s) == aNOSTOP)
	    HasStop = FALSE;
	if (s.type == Lisp::tCONS &&
	    (CAR(s) == aPLATENO || CAR(s) == aID)) { /* for the time being
						    12 January 1998 */
	    SPop(s);
	    StaNo = CAR(s);
	}
    }
	    

    /* New format!
    (SIGNAL 10101 {  2}  {R/L/T/B} (GYR ...) keywords {ST 20} {GT} {ID 2415}
            IJ id opt  
      {PLATE "E2 415"} {MODEL HOME3} {NOSTOP}...)   
    {PLATENO 2415}
	    / * +++++++++++++++++++++process all the other crap here +++++ */

    TSEX endx;
    TrackSeg* ts = FindBranchFromOrientation (tj, orient, endx);
    if (!ts) {
	LispBarf ("Failed to find signal from orientation:", ss);
	return 0;
    }

    HeadsArray head_strings;
    for (;CONSP(Heads); SPop(Heads))
        head_strings.push_back(CAR(Heads).u.a);

    if (StaNo == 0) {
	TrackSegEnd * end = &ts->GetEnd(endx);
	TrackJoint * tj = end->Joint;
	if (tj)
	    StaNo = tj->Nomenclature;
    }
    // Now identical constructors in TLEdit and the main app
    Signal * sig = new Signal (xlno, (int)StaNo, head_strings);
	
    ts->GetEnd(endx).SignalProtectingEntrance = sig;
    /*PanelSignal * Ps = */ new PanelSignal (ts, endx, sig, NULL);
    if (HasStop)
	sig->TStop = new Stop (sig);	/* looks at PanelSignal */

    return 1;
}

#if ! NOAUXK
static int ProcessSwitchkeyForm (Sexpr s) {
    if (ListLen (s) < 3) {
	LispBarf ("Not enough stuff in SWITCHKEY:", s);
	return 0;
    }
    int xno = CAR(s);
    SPop(s);
    WP_cord wpx = CAR(s);
    SPop(s);
    WP_cord wpy = CAR(s);
#if TLEDIT
    (new SwitchKey (xno, wpx, wpy))->Consume();
#else
    (new SwitchKey (NULL, wpx, wpy))->SetXlkgNo(xno);
#endif
    return 1;
}

#endif

static int ProcessTrafficleverForm (Sexpr s) {
    if (ListLen (s) < 5) {
	LispBarf ("Not enough stuff in TRAFFICLEVER:", s);
	return 0;
    }
    Sexpr first = CAR(s);
    SPop(s);
    if (first.type != Lisp::NUM || (int)first != 1) {
	LispBarf ("TRAFFICLEVER version unknown:", s);
	return 0;
    }
    int xno = CAR(s);        SPop(s);
    WP_cord wpx = CAR(s);    SPop(s);
    WP_cord wpy = CAR(s);    SPop(s);
    int rightnormal = CAR(s);
    (new TrafficLever (xno, wpx, wpy, rightnormal))->Consume();
    return 1;
}

static int ProcessPanelLightForm (Sexpr s) {
    if (ListLen (s) < 6) {
	LispBarf ("Not enough stuff in PanelLight:", s);
	return 0;
    }
    Sexpr first = CAR(s);      SPop(s);  /* 1 */

    if (first.type != Lisp::NUM || (int)first != 1) {
	LispBarf ("PanelLight version unknown:", s);
	return 0;
    }
    int xno = CAR(s);       SPop(s); /* 2 */
    int radius = CAR(s);    SPop(s); /* 3 */
    const char * desc = CAR(s).u.s;   SPop(s); /* 4 */
    WP_cord wpx = CAR(s);   SPop(s); /* 5 */
    WP_cord wpy = CAR(s);   SPop(s); /* 6 */

    PanelLight * p = new PanelLight (xno, radius, wpx, wpy, desc);
    for (;CONSP(s); SPop(s)) {
	Sexpr item = CAR(s);
	if (CONSP(item)) {
	    Sexpr color = CAR(item);
	    Sexpr namesym = CADR(item);
	    const char * name = namesym.u.a;
	    p->AddAspect(color.u.a, name);
	}
	/* else diagnose */
    }
    p->Consume();
    return TRUE;
}


static int ProcessPanelSwitchForm (Sexpr s) {
    if (ListLen (s) < 7) {
	LispBarf ("Not enough stuff in PanelSwitch:", s);
	return 0;
    }
    Sexpr first = CAR(s);      SPop(s);  /* 1 */

    if (first.type != Lisp::NUM || (int)first != 1) {
	LispBarf ("PanelSwitch version unknown:", s);
	return 0;
    }
    int xno = CAR(s);       SPop(s); /* 2 */
    SPop(s); /* 3 "posns" (fixnum)*/
    SPop(s); /* 4 "desc" (string)*/

    WP_cord wpx = CAR(s);   SPop(s); /* 5 */
    WP_cord wpy = CAR(s);   SPop(s); /* 6 */

    /* just the nomenclature alphas, not a rlysym */
    const char * rlyname = CAR(s).u.s;   SPop(s); /* 7 */
    (new PanelSwitch (xno, wpx, wpy, rlyname))->Consume();
    return TRUE;
}





static int ProcessViewOriginForm (Sexpr s) {
    if (ListLen (s) < 2) {
	LispBarf ("Not enough stuff in VIEW-ORIGIN:", s);
	return 0;
    }

#if NXSYSMac
    Mac_SetDisplayWPOrg (CAR(s), CADR(s));
#else
    NXGO_SetDisplayWPOrg (CAR(s), CADR(s));
#endif

    return 1;
}

#if REALLY_NXSYS
void SwitchesLoadComplete () {
    for (TrackJoint* joint : SwitchJoints) {
	if (joint->TurnOut)
	    /* ok if happens more than once.for each switch, which
	    it will when gets 53A, 53B, etc */
	    joint->TurnOut->ProcessLoadComplete();
    }
}

#if ! NOAUXK
void AuxKeysLoadComplete () {
    for (TrackJoint* joint : SwitchJoints) {
	Turnout * turnout = joint->TurnOut;
	if (turnout) {
	    int xno = turnout->XlkgNo;
	    SwitchKey* sk = (SwitchKey*) FindHitObject (xno, ID_SWITCHKEY);
	    if (sk)
		sk->AssociateTurnout (turnout);
	}
    }
}
#endif
#endif
