#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <vector>

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



#ifdef TLEDIT
void MacroCleanup() {}; 
#else
#include "xturnout.h"
#include <signal.h>

static std::vector<TrackJoint*> AllJoints;
#endif

#ifdef NXSYSMac
#include "AppDelegateGlobals.h"
#else
#define strdup _strdup
#endif

void XTGLoadClose() {
}

Sexpr read_sexp (FILE* f);		/* oh, what brain-damage! */

#define aDEFLSYM(x) static Sexpr a##x = intern(#x)

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

static Sexpr aVIEW_ORIGIN = intern ("VIEW-ORIGIN");

static std::vector<TrackJoint*> SwitchJoints;


static BOOL DecodeBranchType (Sexpr s, int* brtype) {
    if (s == aSTEM)
	*brtype = TSA_STEM;
    else if (s == aNORMAL)
	*brtype = TSA_NORMAL;
    else if (s == aREVERSE)
	*brtype = TSA_REVERSE;
    else return FALSE;
    return TRUE;
}

void XTGLoadInit() {
}


TrackJoint * FindSwitchJoint (long id, int ab0) {
    TrackJoint * tj;
    for (size_t i = 0; i < SwitchJoints.size(); i++) {
	tj = SwitchJoints[i];
	if (tj->Nomenclature == id && tj->SwitchAB0 == ab0)
	    return tj;
    }
    return NULL;
}

void InitXTGReader ();
int ProcessLayoutForm (Sexpr s);
int ProcessTextForm (Sexpr s);
static int ProcessPathForm (Sexpr s);
static int ProcessSignalForm (Sexpr s);
static int ProcessExitlightForm (Sexpr s);
static int ProcessSwitchkeyForm (Sexpr s);
static int ProcessTrafficleverForm (Sexpr s);
static int ProcessPanelLightForm (Sexpr s);
static int ProcessPanelSwitchForm (Sexpr s);
static int ProcessViewOriginForm (Sexpr s);

void InitXTGReader () {
    SwitchJoints.clear();
#ifndef TLEDIT
    AllJoints.clear();
#endif
}


int XTGLoad (FILE * f) {

    Sexpr s = read_sexp (f);
    if (s.type == L_NULL) {
nlf:	LispBarf (0, "LAYOUT Form missing in file.");
	return 0;
    }
    if (s.type != L_CONS)
	goto nlf;
    if (CAR(s) != aLAYOUT)
	goto nlf;
    InitXTGReader();
    return ProcessLayoutForm (CDR(s));
}

int ProcessLayoutForm (Sexpr f) {
    for ( ;f.type == L_CONS; f = CDR(f)) {
	Sexpr s = CAR(f);
#if 0
	char buf[1000];
	SexpPRep (s, buf);
	MessageBox (0, buf, "Layout subform about to be processed", MB_OK);
#endif
	if (s.type != L_CONS) {
	    LispBarf (1, "Non-list element found in LAYOUT", s);
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
	    LispBarf (1, "Element other than PATH, SIGNAL, EXITLIGHT, SWITCHKEY, TEXT, or VIEW-ORIGIN found in LAYOUT ", s);
	    return 0;
	}
    }
#ifndef TLEDIT
    for (size_t i = 0; i < AllJoints.size(); i++)
		AllJoints[i]->PositionLabel();
	if (SwitchJoints.size() > 0)
		if (!CreateTurnouts(&SwitchJoints[0], (int)SwitchJoints.size()))
			return 0;
#endif
#ifdef NXSYSMac
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
    if (s.type != L_CONS) {
	LispBarf (1, "Non-list where coordinate subform expected:", f);
	return FALSE;
    }
    if (CAR(s).type != L_NUM) {
	LispBarf (1, "Bogus X-coordinate in form", f);
	return FALSE;
    }
    x = CAR(s).u.n;
    s = CDR(s);
    if (s.type == L_CONS) {
	if (CAR(s).type != L_NUM) {
	    LispBarf (1, "Bogus Y-coordinate in switch", f);
	    return FALSE;
	}
	y = CAR(s).u.n;
    }
    else
	y = last_y;

    s = CDR(s);

    if (s.type == L_CONS) {
	if (CAR(s) == aNUMFLIP)
	    flip = TRUE;
    }
    return TRUE;
}

static void SetTCID (TrackSeg * ts, long id) {
    if (id)
#ifdef TLEDIT
	ts->SetTrackCircuit(id, FALSE)->Routed = TRUE;
#else
        ts->SetTrackCircuit(id, FALSE);
#endif
}

static TrackSeg * LinkTS (TrackSeg * last_ts, TrackSeg* ts) {
#ifdef REALLY_NXSYS
    if (last_ts) {
	last_ts->Ends[1].EndIndexNormal = 0;
	last_ts->Ends[1].Next = ts;
    }
    ts->Ends[0].Next = last_ts;
    ts->Ends[0].EndIndexNormal = 1;
#else
    last_ts = ts;
#endif
    return ts;
}


static int ProcessPathForm (Sexpr f) {
    TrackJoint * last_joint = NULL;
    BOOL first = TRUE, last_was_switch = FALSE;
    int last_switch_arc_type = 0;
    struct ppf_coords coords;
    coords.last_y = 0; /* placate flow-analyzer */
    TrackSeg * ts;
    TrackSeg * last_ts = NULL;
    TrackJoint * tj;
    long last_tcid = 0;
    for (; f.type == L_CONS; f = CDR(f)) {
	Sexpr s = CAR(f);
	Sexpr ss = s;
	long Nomen = 0;
	BOOL insulated = FALSE;
	Sexpr key;
	if (s.type == L_NUM) {
	    coords.x = (WP_cord) s.u.n;
	    coords.y = coords.last_y;
	    insulated = FALSE;
create_simple:
#ifdef TLEDIT	    
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
		SetTCID (ts, last_tcid);

		if (tj)
		    tj->AddBranch(ts);
		ts->Ends[0].Joint = last_joint;
		ts->Ends[1].Joint = tj;

		if (last_was_switch)
		    last_joint->TSA[last_switch_arc_type] = ts;
		else
		    if (last_joint)
			last_joint->AddBranch (ts);
	    }
#ifdef REALLY_NXSYS
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

	if (s.type != L_CONS) {
	    LispBarf (1, "Element other than number or list in PATH", s);
	    return 0;
	}
	key = CAR(s);
	if (key.type == L_NUM) {
	    coords.x = key.u.n;
	    if (CDR(s).type == L_CONS) {
		if (CADR(s).type != L_NUM) {
		    LispBarf (1, "Coordinate-pair form in PATH not a pair of numbers.", s);
		    return 0;
		}
	    }
	    coords.y = CADR(s).u.n;
	    insulated = FALSE;
	    goto create_simple;
	}
	    
	if (key.type != L_ATOM) {
	    LispBarf (1, "Form with non atomic head in PATH ", key);
	    return 0;
	}
	s = CDR(s);
	if (key == aIJ) {
	    if (ListLen(s) < 2) {
		LispBarf (1, "Not enough data in IJ description ", s);
		return 0;
	    }
	    insulated = TRUE;
	    Nomen = CAR(s).u.n;
	    if (!coords.Collect (CDR(s), ss))
		return 0;
	    goto create_simple;
	}
	else if (key == aTC) {
	    if (ListLen(s) < 1) {
		LispBarf (1, "Not enough data in TC description ", ss);
		return 0;
	    }
	    last_tcid = CAR(s).u.n;
	    continue;
	}
	else if (key != aSWITCH) {
	    LispBarf (1, "Unrecognized subform in PATH ", key);
	    return 0;
	}

	/* (SWITCH) BRANCHTYPE swID x y */
	if (s.type != L_CONS) {
invsw:	    LispBarf (1, "Invalid SWITCH subform", s);
	    return 0;
	}
	if (CAR(s).type != L_ATOM)
	    goto invsw;
	int brtype;
	if (!DecodeBranchType (CAR(s), &brtype)) {
	    LispBarf (1, "Unknown switch branch type", CAR(s));
	    return 0;
	}
	if (s.type != L_CONS) {
	    LispBarf (1, "Missing nomenclature in switch", ss);
	    return 0;
	}

	s = CDR(s);

	if (CAR(s).type != L_NUM) {
	    LispBarf (1, "Bogus Nomenclature ID switch", ss);
	    return 0;
	}
	Nomen = CAR(s).u.n;
	s = CDR(s);
	
	int ab0;
	Sexpr e = CAR(s);

	if (e.type == L_NUM) {
	    if (e.u.n != 0) {
unkab:		LispBarf (1, "Unknown Switch A/B/singleton tag:", ss);
		return 0;
	    }
	    ab0 = 0;
	}
	else if (e == aA)
	    ab0 = 1;
	else if (e == aB)
	    ab0 = 2;
	else goto unkab;
	s = CDR(s);

	BOOL have_coords = FALSE;

	if (s.type == L_CONS) {
	    if (!coords.Collect (s, ss))
		return 0;
	    have_coords = TRUE;
	}

	TrackJoint * swj = FindSwitchJoint (Nomen,ab0);
	if (swj != NULL) {
	    if (!(first || CDR(f) == NIL)) {
		LispBarf (1, "Previously-defined switch neither first nor last", ss);
		return 0;
	    }

	}
	if (swj == NULL) {
	    if (!have_coords) {
		LispBarf (1, "Switch Undefined: point coordinates missing", ss);
		return 0;
	    }
	    swj = new TrackJoint (coords.x, coords.y);
#ifdef REALLY_NXSYS
	    AllJoints.push_back(swj);
#endif
#ifdef TLEDIT	    
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
	    swj->TSA[brtype] = ts;
	    ts->Ends[1].Joint = swj;
	    ts->Ends[0].Joint = last_joint;
#ifndef xTLEDIT
	    if (last_was_switch)
		last_joint->TSA[last_switch_arc_type] = ts;
	    else
		if (last_joint)
		    last_joint->AddBranch (ts);
#endif
	    last_switch_arc_type
		    = (brtype == TSA_STEM) ? TSA_NORMAL : TSA_STEM;
	}
/* NEED WHOLE BUSINESS FOR LINKING THESE FUCKERS WITHOUT TJ NODES */
	last_joint = swj;
	last_was_switch = TRUE;
	goto finish_create_any;
    }
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
    
static TrackSeg * FindBranchFromOrientation (TrackJoint * tj,
					     char orient, int& ex) {
    for (int i = 0; i < tj->TSCount; i++) {
	TrackSeg * ts = tj->TSA[i];
	ex = tj->FindEndIndex(ts);
	//TrackSegEnd * ep = &ts->Ends[ex];
	if (tj->TSCount == 1 || SigMatchOrientation (orient, ts, ex))
	    return ts;
    }
    return NULL;
}


static TrackJoint * FindTrackJoint (long nn) {
#ifdef TLEDIT
    return (TrackJoint *) FindHitObject (nn, ID_JOINT);
#else
    for (size_t i = 0; i < AllJoints.size(); i++)
	if (AllJoints[i]->Nomenclature == nn)
	    return AllJoints[i];
    return NULL;
#endif
}


static int ProcessExitlightForm (Sexpr s) {
    if (s.type != L_CONS) {
	LispBarf (0, "EXITLIGHT form missing data.");
	return 0;
    }
    if (CAR(s).type != L_NUM) {
	LispBarf (0, "EXITLIGHT form missing lever number.");
	return 0;
    }
    long xno = CAR(s).u.n;
    SPop(s);
    if (s == NIL) {
	PanelSignal * ps = (PanelSignal*) FindHitObject (xno, ID_SIGNAL);
	if (!ps) {
	    LispBarf (1, "Cannot find signal for EXITLIGHT", CreateFixnum(xno));
	    return 0;
	}
	ps->Seg->Ends[ps->EndIndex].ExLight
		= new ExitLight (ps->Seg, ps->EndIndex, (int)xno);
	return 1;
    }
    /* (xno orient ij) */
    Sexpr e = CAR(s);
    char orient;
    if (e.type == L_ATOM) {
	if (e != aL && e != aR && e != aT && e != aB) {
	    LispBarf (2, "Unrecognized orientation symbol in EXITLIGHT",
		      CreateFixnum (xno), e);
	    return 0;
	}
	orient = e.u.s[0];
	SPop(s);
	long nn = CAR(s).u.n;
	TrackJoint * tj = FindTrackJoint (nn);
	if (!tj) {
	    LispBarf (1, "Cannot find Joint for EXITLIGHT", CAR(s));
	    return 0;
	}
	int ex;
	TrackSeg * ts = FindBranchFromOrientation (tj, orient, ex);
	if (!ts) {
	    LispBarf (0, "Cannot find EXITLIGHT IJ/orient reference.");
	    return 0;
	}
	ts->Ends[ex].ExLight = new ExitLight (ts, ex, (int)xno);
	return 1;
    }
    LispBarf (0, "Bogus EXITLIGNT item", s);
    return 0;
}

static int ProcessSignalForm (Sexpr s) {
    Sexpr ss = s;
    long ijid = 0;
    Sexpr Heads = NIL;
    int xlno = 0;
    char orient = ' ';
    BOOL HasStop = TRUE;

    if (CAR(s).type != L_NUM) {
	LispBarf (1, "IJ id number missing in SIGNAL form", ss);
	return 0;
    }
    ijid = CAR(s).u.n;
    SPop(s);
    if (CAR(s).type == L_NUM) {
	xlno= (int)CAR(s).u.n;
	SPop(s);
    }
    Sexpr e = CAR(s);
    if (e.type == L_ATOM) {
	if (e != aL && e != aR && e != aT && e != aB) {
	    LispBarf (2, "Unrecognized orientation symbol in SIGNAL",
		      e, ss);
	    return 0;
	}
	orient = e.u.s[0];
	SPop(s);
    }
    if (CAR(s).type != L_CONS) {
	LispBarf(1, "Missing heads list in SIGNAL", ss);
	return 0;
    }
    Heads = CAR(s);
    long StaNo = 0L;
    TrackJoint * tj = FindTrackJoint (ijid);
    if (tj == NULL) {
	LispBarf (1, "Cannot find Insulated Joint", ss);
	return 0;
    }
    while (s.type == L_CONS) {
	SPop(s);
	if (s.type == L_CONS && CAR(s) == aNOSTOP)
	    HasStop = FALSE;
	if (s.type == L_CONS &&
	    (CAR(s) == aPLATENO || CAR(s) == aID)) { /* for the time being
						    12 January 1998 */
	    SPop(s);
	    StaNo = CAR(s).u.n;
	}
    }
	    

    /* New format!
    (SIGNAL 10101 {  2}  {R/L/T/B} (GYR ...) keywords {ST 20} {GT} {ID 2415}
            IJ id opt  
      {PLATE "E2 415"} {MODEL HOME3} {NOSTOP}...)   
    {PLATENO 2415}
	    / * +++++++++++++++++++++process all the other crap here +++++ */

    int ex;
    TrackSeg* ts = FindBranchFromOrientation (tj, orient, ex);
    if (!ts) {
	LispBarf (1, "Failed to find signal from orientation:", ss);
	return 0;
    }


#ifdef TLEDIT
    Signal * sig = new Signal;
    sig->XlkgNo = xlno;
    if (Heads == NIL)
	sig->HeadsString = strdup ("");
    else {
	char buf[200];
	SexpPRep (Heads, buf);
	size_t l = strlen(buf);
	if (buf[l-1] == ')')
	    buf[l-1] = '\0';
	if (buf[0] == '(')
	    sig->HeadsString = strdup (buf+1);
	else
	    sig->HeadsString = strdup (buf);
    }
    sig->StationNo = (int)StaNo;
#else

    char *headarray[6];
    int head_ct;
    for (head_ct = 0;CONSP(Heads); SPop(Heads),head_ct++)
	headarray[head_ct] = _strdup (CAR(Heads).u.a);

    if (StaNo == 0) {
	TrackSegEnd * end = &ts->Ends[ex];
	TrackJoint * tj = end->Joint;
	if (tj)
	    StaNo = tj->Nomenclature;
    }

    Signal * sig = new Signal (NULL, (int)StaNo, head_ct, headarray, xlno, 0);
#endif
	
    ts->Ends[ex].SignalProtectingEntrance = sig;
    /*PanelSignal * Ps = */ new PanelSignal (ts, ex, sig, NULL);
    if (HasStop)
	sig->TStop = new Stop (sig);	/* looks at PanelSignal */

    return 1;
}

#ifndef NOAUXK
static int ProcessSwitchkeyForm (Sexpr s) {
    if (ListLen (s) < 3) {
	LispBarf (1, "Not enough stuff in SWITCHKEY:", s);
	return 0;
    }
    int xno = (int)CAR(s).u.n;
    SPop(s);
    WP_cord wpx = CAR(s).u.n;
    SPop(s);
    WP_cord wpy = CAR(s).u.n;
#ifdef TLEDIT
    new SwitchKey (xno, wpx, wpy);
#else
    (new SwitchKey (NULL, wpx, wpy))->SetXlkgNo(xno);
#endif
    return 1;
}

#endif

static int ProcessTrafficleverForm (Sexpr s) {
    if (ListLen (s) < 5) {
	LispBarf (1, "Not enough stuff in TRAFFICLEVER:", s);
	return 0;
    }
    Sexpr first = CAR(s);
    SPop(s);
    int vno = (int)first.u.n;

    if (vno != 1 || first.type != L_NUM) {
	LispBarf (1, "TRAFFICLEVER version unknown:", s);
	return 0;
    }
    int xno = (int)CAR(s).u.n;
    SPop(s);
    WP_cord wpx = CAR(s).u.n;
    SPop(s);
    WP_cord wpy = CAR(s).u.n;
    SPop(s);
    int rightnormal = (int)CAR(s).u.n;
    new TrafficLever (xno, wpx, wpy, rightnormal);
    return 1;
}

static int ProcessPanelLightForm (Sexpr s) {
    if (ListLen (s) < 6) {
	LispBarf (1, "Not enough stuff in PanelLight:", s);
	return 0;
    }
    Sexpr first = CAR(s);      SPop(s);  /* 1 */
    int vno = (int)first.u.n;

    if (vno != 1 || first.type != L_NUM) {
	LispBarf (1, "PanelLight version unknown:", s);
	return 0;
    }
    int xno = (int)CAR(s).u.n;       SPop(s); /* 2 */
    int radius = (int)CAR(s).u.n;    SPop(s); /* 3 */
    char * desc = CAR(s).u.s;   SPop(s); /* 4 */
    WP_cord wpx = CAR(s).u.n;   SPop(s); /* 5 */
    WP_cord wpy = CAR(s).u.n;   SPop(s); /* 6 */

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
    return TRUE;
}


static int ProcessPanelSwitchForm (Sexpr s) {
    if (ListLen (s) < 7) {
	LispBarf (1, "Not enough stuff in PanelSwitch:", s);
	return 0;
    }
    Sexpr first = CAR(s);      SPop(s);  /* 1 */
    int vno = (int)first.u.n;

    if (vno != 1 || first.type != L_NUM) {
	LispBarf (1, "PanelSwitch version unknown:", s);
	return 0;
    }
    int xno = (int)CAR(s).u.n;       SPop(s); /* 2 */
    SPop(s); /* 3 "posns" (fixnum)*/
    SPop(s); /* 4 "desc" (string)*/

    WP_cord wpx = CAR(s).u.n;   SPop(s); /* 5 */
    WP_cord wpy = CAR(s).u.n;   SPop(s); /* 6 */

    const char * rlyname = CAR(s).u.s;   SPop(s); /* 7 */
    new PanelSwitch (xno, wpx, wpy, rlyname);
    return TRUE;
}





static int ProcessViewOriginForm (Sexpr s) {
    if (ListLen (s) < 2) {
	LispBarf (1, "Not enough stuff in VIEW-ORIGIN:", s);
	return 0;
    }

#ifdef NXSYSMac
    Mac_SetDisplayWPOrg (CAR(s).u.n, CADR(s).u.n);
#else
    NXGO_SetDisplayWPOrg (CAR(s).u.n, CADR(s).u.n);
#endif

    return 1;
}

#ifdef REALLY_NXSYS
void SwitchesLoadComplete () {
    for (size_t i = 0; i < SwitchJoints.size(); i++) {
	if (SwitchJoints[i]->TurnOut)
	    /* ok if happens more than once.for each switch, which
	    it will when gets 53A, 53B, etc */
	    SwitchJoints[i]->TurnOut->ProcessLoadComplete();
    }
}

#ifndef NOAUXK
void AuxKeysLoadComplete () {
    for (size_t i = 0; i < SwitchJoints.size(); i++) {
	Turnout * t = SwitchJoints[i]->TurnOut;
	if (t) {
	    int xno = t->XlkgNo;
	    SwitchKey* sk = (SwitchKey*) FindHitObject (xno, ID_SWITCHKEY);
	    if (sk)
		sk->AssociateTurnout (t);
	}
    }
}
#endif
#endif
