#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "lisp.h"
#include "stdio.h"
#include "track.h"
#include "signal.h"
#include "stop.h"
#include "relays.h"
#include <ctype.h>
#include "compat32.h"
#include "lispmath.h"
#include "objid.h"
#include "traincfg.h"
#include "staplat.h"
#include "lyglobal.h"
#include "nxglapi.h"
#include "rlyapi.h"
#include "trafficlever.h"

#include "rdtrkv1.h"

#define RTKLE "Read Track Layout Error"
#define BARF(x) {MessageBox(0,x,RTKLE,MB_OK);return 0;}
extern char XGlobalRoutid;
extern int Station_base;
int symcmp (Sexpr& s, char * str);

int ProcessPlatForm (Sexpr s) {	/* hahaha */
    /* (PLATFORM island 44200 44700   15     75     "Foobar St.") */
    /*            type  x1*100 x2*100 y1*100 y2*100 string */
    Sexpr type = CAR(s); SPop(s);
    PlatformType t= PLATYPE_ISLAND;
    if (symcmp (type, "ISLAND"))
	t = PLATYPE_ISLAND;
    else if (symcmp (type, "TOP"))
	t = PLATYPE_TOP;
    else if (symcmp (type, "BOTTOM")) {/* TBD*/ };
    long x1 = CAR(s).u.n; SPop(s);
    long x2 = CAR(s).u.n; SPop(s);
    long y1 = CAR(s).u.n; SPop(s);
    long y2 = CAR(s).u.n; SPop(s);
    char * legend = NULL;
    if (s != NIL)
	legend = CAR(s).u.s;
    /* the *100 is real crap - need flonums in obj file */
    LayoutPlatform (t, x1*100, x2*100, y1, y2, legend);
    return 1;
}


static void LayoutTrack (TrackDef * tk, Sexpr IJl, int track_index) {
    for (;CONSP(CDR(IJl)); SPop (IJl)) {
	int ijplace = CAR(IJl).u.n;
	int ijend = CAR(CDR(IJl)).u.n;
	RW_cord rwx = (RW_cord) ijplace * 100;
	RW_cord rwx2 = (RW_cord) ijend * 100;
	TrackSec * tr = new TrackSec (rwx, rwx2, track_index, ijplace, ijend, tk);
	long trelay_no = compute_track_relayno (tk->TrackNo, tr->Station_No);
	tr->Relay = CreateAndSetReporter
		    (trelay_no, "T", TrackSec::TrackReportFcn, tr);
	CreateAndSetReporter (trelay_no, "K", TrackSec::TrackKRptFcn, tr);
    }
    tk->EndSno = tk->LastSno = CAR(IJl).u.n;
}

static void LayoutTurnout (char rte, int t1, int s1, int t2, int s2,
		    int xno, int singleton) {
    TrackSec *ts1 = FindContainingTrackSec (rte, t1, s1);
    TrackSec *ts2 = FindContainingTrackSec (rte, t2, s2);
    Turnout * tn = new Turnout (ts1, s1, ts2, s2, xno);
    tn->Singleton = singleton;
    if (singleton)
	ts2->AdjustSingleton (((RW_cord) s2 * 100), (s1 > s2));
}

long GetCanonicalSignalNumber (Signal * g) {
    return g->XlkgNo ? g->XlkgNo :
#ifdef XTG
	    g->StationNo;
#else
	    compute_track_relayno (g->ForwardTS->Track->TrackNo, g->StationNo);
#endif
}

void LayoutSignal (char rte, int tno, Sexpr SInfo, Sexpr heads, Sexpr rest) {
    int sno, pos_sno;
    BOOL rdp;
    if (SInfo.type == L_NUM) {
	sno = pos_sno = SInfo.u.n;
	rdp = FALSE;
    }
    else {
	if (symcmp (CAR(SInfo), "REVDIR")) {
	    pos_sno = CADR(SInfo).u.n;
	    sno = CADDR(SInfo).u.n;
	    rdp = TRUE;
	}
	else if (symcmp (CAR(SInfo), "DISPLACE")) {
	    pos_sno = CADR(SInfo).u.n;
	    sno = CADDR(SInfo).u.n;
	    rdp = FALSE;
	}
    }
    TrackSec *ts = FindContainingTrackSec (rte, tno, pos_sno);
    if (rdp) ts->DwarfDiddle();

    if (rdp)
	if (ts->NominalSouthp)
	    ts = ts->North;
	else ts = ts->South;

    int xno = 0;
    if (CONSP(rest))
	if (CAR(rest).type == L_NUM) {
	    xno = CAR(rest).u.n;
	    SPop (rest);
	}
    int nostop = 0;
    if (CONSP(rest))
	nostop = symcmp(CAR(rest), "NOSTOP");
    char *headarray[6];
    for (int head_ct = 0;CONSP(heads); SPop(heads),head_ct++)
	headarray[head_ct] = strdup (CAR(heads).u.a);

    Signal *g = new Signal (ts, sno, head_ct, headarray, xno, rdp);
    g->RealStationPos = pos_sno;
    if (!nostop)
	new Stop(g);
}

static void LayoutExitLight (char rte, int xno, int tkno, int stno, int southp) {
    TrackSec *ts = FindContainingTrackSec (rte, tkno, stno);
    TrackDef *td = ts->Track;
    new ExitLight (td, stno*100L, ts->rw_y, southp, xno);
}

int ProcessSwitchForm (Sexpr s) {
    int t1 = CAR(s).u.n; SPop(s);
    int s1 = CAR(s).u.n; SPop(s);
    int t2 = CAR(s).u.n; SPop(s);
    int s2 = CAR(s).u.n; SPop(s);
    int xno = CAR(s).u.n; SPop(s);
    int singleton =  (s != NIL && (symcmp (CAR(s), "SINGLETON")));
    LayoutTurnout (XGlobalRoutid, t1, s1, t2, s2, xno, singleton);
    return 1;
}

int ProcessSignalForm (Sexpr s) {
    int tno = CAR(s).u.n; SPop(s);
    Sexpr SInfo = CAR(s); SPop(s);
    Sexpr heads = CAR(s); SPop(s);
    LayoutSignal (XGlobalRoutid, tno, SInfo, heads, s);
    return 1;
}

int ProcessExitLightForm (Sexpr s) {
    int xno = CAR(s).u.n; SPop(s);
    int tkno = CAR(s).u.n; SPop(s);
    int stno = CAR(s).u.n; SPop(s);
    int southp = symcmp (CAR(s), "SOUTH");
    LayoutExitLight (XGlobalRoutid, xno, tkno, stno, southp);
    return 1;
}

int ProcessTrackForm (Sexpr s) {
    if (CAR(s).type != L_NUM)
	BARF ("Track number not a number.");
    int track_index = Glb.TrackDefCount;

    int track_number = CAR(s).u.n;
    TrackDef * tk = new TrackDef (XGlobalRoutid, track_number, Station_base);
    SPop(s);
    Sexpr subl = CAR(s);
    long tlabpos = -1;
    if (subl.type != L_CONS)
	BARF ("Track subspec not list.");
    if (CAR(subl).type == L_ATOM && symcmp (CAR(subl), "TLABEL")) {
	tlabpos = CADR(subl).u.n;
	SPop (s);
	subl = CAR(s);
    }
    if (subl.type != L_CONS)
	BARF ("Track subspec not list.");
    LayoutTrack (tk, subl, track_index);
    if (tlabpos > -1)
	new TrackLabel (tk, tlabpos*100L, track_index);
    return 1;
}


int ProcessTrafficleverForm (Sexpr s) {
    if (ListLen (s) < 5) {
	BARF ("Not enough stuff in TRAFFICLEVER form (need 5)");
	return 0;
    }
    Sexpr first = CAR(s);
    SPop(s);
    int vno = first.u.n;

    if (vno != 1 || first.type != L_NUM) {
	BARF ("TRAFFICLEVER version unknown");
	return 0;
    }
    int xno = CAR(s).u.n;
    SPop(s);
    int x = CAR(s).u.n*100;
    SPop(s);
    int y = CAR(s).u.n;  /* NOT *100'ed */
    SPop(s);
    int rightnormal = CAR(s).u.n;
    TrackDef * td = TrackDefs[0];	/* better be one */
    WP_cord wp_x = RWx_to_WPx (td, x);
    WP_cord wp_y = RWyhun_to_WPy (y);

    new TrafficLever (xno, wp_x, wp_y, rightnormal);
    return 1;
}
