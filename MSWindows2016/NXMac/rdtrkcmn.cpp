#include "windows.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "lisp.h"

#define RGP_BOBBLE_TIME_MS 3000

#ifndef XTG
#include "track.h"
#endif

#include "signal.h"
#include "relays.h"
#include <ctype.h>
#include "compat32.h"
#include "lispmath.h"
#include "objid.h"
#include "traincfg.h"
#include "incexppt.h"
#include "nxglapi.h"
#include "nxsysapp.h"
#include "timers.h"
#include "rlyapi.h"
#include "usermsg.h"
#include "trafficlever.h"
#include "helpdlg.h"


#ifdef XTG
#include "xtgtrack.h"
#include "xtgload.h"
#include "xturnout.h"
void SetTrackGeometryDefaults();
#else
#include <stop.h>
#include "rdtrkv1.h"
int Station_base;
#endif

#include "lyglobal.h"
#include "loaddcls.h"

#ifdef NXV2
#include "dynmenu.h"
#endif

void ValidateRelayWorld();

char  InterlockingName[512] = {'\0'};
char  InterpretedP = 0;
char  InterlockingLoaded = 0;
char  XGlobalRoutid = '?';
Relay *CPB0 = NULL;
const char  *INameRetval;

Sexpr read_sexp (FILE* f);
void ReportAllTrackSecsClear ();
void ReportAllTrafficLeversNormal();
int LoadRelayObjectFile (const char * ref, const char * fn);
void CreateSwitchKeys();
void InitSwitchKeyData();
void InitTrafficLeverData();
void ProcessLoadComplete();
void Goose_Generale (void *v);
int InterpretTopLevelForm (const char * fname, Sexpr s);
int ProcessRouteForm (Sexpr s, const char * ref_fname);


static Relay * BRGP0 = NULL;
static Relay * RAS0 = NULL;

/* forward def */
void ProcessLoadComplete();

#define LERROR return LoadError
static int LoadError (const char * msg, Sexpr s) {
    char buf[1024];
    strcpy (buf, msg);
    if (s != EOFOBJ) {
	strcat (buf, ": ");
	SexpPRep (s, buf+strlen(buf));
    }
    usermsgstop ("Error in track code file: %s", buf);
    return 0;
}

static void FinishUpLoad () {
    ProcessLoadComplete();
#ifndef NOAUXK
#ifdef NXV2
    AuxKeysLoadComplete();
#else
    CreateSwitchKeys();
#endif
#endif
    InterlockingLoaded = 1;
    ReportAllTrackSecsClear ();
    ReportAllTrafficLeversNormal();
    void ReportAllTrafficLeversNormal();
    BRGP0 = CreateQuislingRelay (0, "BRGP");
    RAS0 = CreateQuislingRelay (0, "RAS");
    CPB0 = CreateQuislingRelay (0, "CPB");

#ifdef NXV2
    EnableDynMenus(TRUE);
#ifndef NOTRAINS
    SetUpLayoutTrainMetrics();
#endif
#endif
    map_relay_syms (Goose_Generale);
#ifdef NXV2
    DropAllApproach();
#endif

}


/* this is a big crock.  Some much better theory is necessary */

static const char *NormallyPickedRelays[]
   = {"AS", "D", "DV", "RGP", NULL};

int symcmp (Sexpr& s, const char * str) {
    if (s.type != L_ATOM)
	return 0;
    return (!strcmp (s.u.a, str));
}

 void Goose_Generale (void *v) {
    Rlysym *rs = (Rlysym *) v;
    const char * nam = redeemRlsymId (rs->type);
    const char ** magicnames;
    for (magicnames = NormallyPickedRelays;
	 *magicnames != NULL; magicnames++)
	if (!strcmp (nam, *magicnames))
	    if (rs->rly)		/* skip garbage syms from macros */
		GooseRelay (rs->rly);
}


#ifdef NXOGL

static int DefineSignalsNXGL (GraphicObject * go) {
    NXGLDefineSignal ((Signal *) go);	/* very boring at 16 bits... */
    return 0;
}

BOOL FeedLayoutToNXGL () {
    if (Glb.TrackDefCount == 0)
	return FALSE;
    NXGLDefineSigStateFcn (Signal::SigStateExt);
    NXGLDefineLayout (&Glb); /* noop at 16 bits */
    MapGraphicObjectsOfType (ID_SIGNAL, DefineSignalsNXGL);
    int lasttk = Glb.TrackDefCount - 1;
    if (lasttk == 3)
	lasttk = 2;
    NXGLCreateView(TrackDefs[lasttk], TrackDefs[lasttk]->Station_base, 0);
    return TRUE;
}
#endif

static BOOL LoadExprcodeFile (const char * fname) {
    FILE* f = fopen (fname, "r");
    if (f == NULL) {
	usermsgstop ("Cannot open exprcode track file %s for reading: %s",
		     fname, strerror(errno));
	return FALSE;
    }
    //printf("Load exprcode file %s\n", fname);
    HCURSOR hc = SetCursor (LoadCursor (NULL, IDC_WAIT));
    BOOL got_it = FALSE;
    BOOL success = TRUE;
    while (success && !got_it) {

       // ValidateRelayWorld();
	Sexpr s = read_sexp (f);
        //show_sexp(s);
 //       ValidateRelayWorld();
	if (s.type == L_NULL)
	    if (s == EOFOBJ)
		got_it = TRUE;	/* otherwise read_err_obj */
	    else
		success = FALSE;
	else
	    success = (BOOL) InterpretTopLevelForm (fname, s);
        

   //     ValidateRelayWorld();
	dealloc_ncyclic_sexp (s);
   //     ValidateRelayWorld();
    }
    fclose(f);
    SetCursor (hc);
    return got_it;
}


const char * ReadLayout (const char* fname) {
    INameRetval = "NX Interlocking";
    InitRelaySys();
#ifdef XTG
    InitXTGReader();
    EnableDynMenus(FALSE);
    TrackCircuitSystemReInit();
    InitSwitchKeyData();
#endif
    InitTrafficLeverData();
#ifndef NXV2
    InitLispSys();			/* in V2, autoinit symbols */
    Glb.TrackDefCount = 0;		/* might free old some day */
#endif
    char ext [MAXEXT];
    fnsplit (fname, NULL, NULL, NULL, ext);

    if (!_stricmp (ext, ".TKO")) {
#ifdef NXSYSMac
        std::string em;
        em += fname;
        em += "\n";
        em +=".TKO compiled interlockings are not supported on NXSYS/Mac.";
        MessageBox(NULL, em.c_str(), "NXSYS Layout Loader", MB_ICONSTOP | MB_OK);
        return NULL;
#elif defined(NXCMPOBJ)
	HCURSOR hc = SetCursor (LoadCursor (NULL, IDC_WAIT));
	if (!LoadRelayObjectFile (fname, fname)) {
	    SetCursor (hc);
	    /* We should have already produced a meaningful message */
	    /* Getting it back to OLE automation is still an unsolved
	    problem...*/
	    return NULL;
	}
	SetCursor (hc);
	strcpy (InterlockingName, INameRetval);
	InterpretedP = 0;
#else
	std::string em;
	em += fname;
	em += "\n";
	em += ".TKO compiled interlockings are not supported by this version of NXSYS.";
	MessageBox(NULL, em.c_str(), "NXSYS Layout Loader", MB_ICONSTOP | MB_OK);
	return NULL;
#endif
    }
    else {
	if (!LoadExprcodeFile (fname)){
	    DeInstallLayout();
	    return NULL;
	}
	InterpretedP = 1;
	strcpy (InterlockingName, INameRetval);
	const char * nn = " (Interpreted)";
	char * nt = (char *) malloc (strlen (INameRetval) + strlen(nn)+1);
	INameRetval = strcat (strcpy (nt, INameRetval), nn);
    }
    FinishUpLoad();
    return (char *)INameRetval; ////+++++REALLY BAD CAST TBF
}

static void ResetRouteDefaults() {
    Glb.TorontoStyle = FALSE;
    Glb.IRTStyle = FALSE;
    Glb.RightIsSouth = FALSE;
    SetTrackGeometryDefaults();
#ifndef NOTRAINS
    SetTrainKinematicsDefaults();
#endif
}

extern "C" int malloc_zone_check(int );
int InterpretTopLevelForm (const char* fname, Sexpr s) {

#if 0  //HEAPCHK
    int * pp = 0;
    int c = malloc_zone_check(0);
    if (c != 1)
        c = *pp;
#endif
    if (s.type != L_CONS)
	LERROR ("Item definition not a list", s);
    Sexpr fn = CAR(s);
    int subll = ListLen(s);
    if (subll < 1)
	LERROR ("Top Level Item has fewer than one elements", s);
    if (fn.type != L_ATOM)
	LERROR ("Top-level item doesn't start with atom", s);
    Sexpr f2 = MaybeExpandMacro (s);
    if (f2 != EOFOBJ) {
	int v = InterpretTopLevelForm (fname, f2);
	dealloc_ncyclic_sexp (f2);
	return v;
    }
    else if (fn == DEFRMACRO) {
	defrmacro (s);
	return 1;
    }
    else if (fn == FORMS) {
	SPop (s);
forms:
	while (s.type == L_CONS) {
	    if (!InterpretTopLevelForm (fname, CAR(s)))
		return 0;
	    SPop (s);
	}
	return 1;
    }
    Sexpr rest = CDR(s);

    if (symcmp (fn, "QUIT"))
	return 1;
#ifdef NXCMPOBJ
    else if (symcmp (fn, "LOAD")) {
        std::string pathbuf;
	return LoadRelayObjectFile
		(fname, include_expand_path (fname, CADR(s).u.s, pathbuf));
    }
#endif
    else if (symcmp (fn, "INCLUDE")) {
        std::string pathbuf;
	return LoadExprcodeFile (include_expand_path (fname, CADR(s).u.s, pathbuf));
    }
    else if (symcmp (fn, "RELAY"))
	return (DefineRelayFromLisp (rest) != NULL);
    else if (symcmp (fn, "TIMER"))
	return (DefineTimerRelayFromLisp (rest) != NULL);
    else if (symcmp (fn, "ROUTE")) {
	ResetRouteDefaults();
	return ProcessRouteForm (rest, fname);
    }
#ifdef NXV2
    else if (symcmp (fn, "MENU"))
	return DefineMenuFromLisp(rest);
#endif
    else if (symcmp (fn, "COMMENT"))
	return 1;
    else if (symcmp (fn, "EVAL-WHEN")) {
	SPop(s);
	if (s.type == L_CONS && CAR(s).type == L_CONS) {
	    Sexpr sc = CAR(s);
	    SPop(s);
	    for (; sc.type == L_CONS; SPop(sc))
		if (CAR(sc).type == L_ATOM && symcmp (CAR(sc), "EVAL"))
		    goto forms;
	}
	return 1;
    }
    else if (symcmp (fn, "LAYOUT"))
#ifdef NXV2
	return ProcessLayoutForm (rest);
#else
	LERROR ("This is a Version 2 NXSYS layout. You cannot use it with Version 1 NXSYS.", EOFOBJ);
    else if (symcmp (fn, "TRACK"))
	return ProcessTrackForm(rest);
    else if (symcmp (fn, "SWITCH"))
	return ProcessSwitchForm (rest);
    else if (symcmp (fn, "SIGNAL"))
	return ProcessSignalForm (rest);
    else if (symcmp (fn, "EXITLIGHT"))
	return ProcessExitLightForm (rest);
    else if (symcmp (fn, "PLATFORM"))
	return ProcessPlatForm (rest);
    else if (symcmp (fn, "TEXT"))
	return ProcessTextForm (rest);
    else if (symcmp (fn, "TRAFFICLEVER"))
	return ProcessTrafficleverForm (rest);
#endif
    LERROR ("Unknown top level form", fn);
}

long Signal::CanonicalNumber () {
    return XlkgNo ? XlkgNo :
#ifdef XTG
	    StationNo;
#else
	    compute_track_relayno (ForwardTS->Track->TrackNo, StationNo);
#endif
}

static void BobbleTimeFun (void *) {
    ReportToRelay (BRGP0, FALSE);
}

void BobbleRGPs() {
    if (BRGP0 != NULL) {
	ReportToRelay (BRGP0, TRUE);
	NXTimer (BRGP0, BobbleTimeFun, RGP_BOBBLE_TIME_MS);
    }
}

void DropAllApproach () {
    if (RAS0 != NULL)
	PulseToRelay (RAS0);
}

Relay * CreateQuislingRelay (long xno, const char * nomenclature) {
    return CreateRelay (intern_rlysym (xno, nomenclature));
}

ReportingRelay * CreateAndSetReporter
   (long xno, const char * nomenclature, RelRptFcn f, void * object) {
    Sexpr rlysym = intern_rlysym (xno, nomenclature);
    ReportingRelay * rly = CreateReportingRelay (rlysym);
    rly->SetReporter (f, object);
    return rly;
}

ReportingRelay * ReportingRelayNoCreate (long xlkgno, const char * s) {
    /* should diagnose if it's not really a "reporting"-type relay */
    return (ReportingRelay *) GetRelay2NoCreate(xlkgno, s);
}

BOOL SetReporterIfExists (long itemno, const char * s, RelRptFcn f, void * obj) {
    ReportingRelay * r = ReportingRelayNoCreate (itemno, s);
    if (r) {
	r->SetReporter (f, obj);
	return TRUE;
    }
    return FALSE;
}

void RelaySetReporter (Relay * r, RelRptFcn f, void * obj) {
    ((ReportingRelay *) r)->SetReporter (f, obj);
}

BOOL Signal::HomeP() {
    for (int h = 1; h < HeadCount; h++)
	if (Heads[h]->height > 2)
	    return TRUE;
    return FALSE;
}

void Signal::ProcessLoadComplete () {
    long gno = CanonicalNumber();
    /*pbs/bpbs gets done below */
    if (XlkgNo) {
	PB  = CreateQuislingRelay (gno, "PB");
	FL  = CreateQuislingRelay (gno, "FL");
    }
    else Signal::Selected = 1;   /* auto: interlocking, not NXGO sense */

    BOOL interlocked = XlkgNo != 0;

    for (int h = 1; h < HeadCount; h++) {
	if (Heads[h]->height > 2) {
	    CreateAndSetReporter (gno, "DR", Signal::DivReporter, this);
	    COPB = CreateQuislingRelay (gno, "COPB");
	    CreateAndSetReporter (gno, "CO", Signal::COReporter, this);
	}
	switch (Heads[h]->Lights[0]) {
	    case 'S':
		CreateAndSetReporter (gno, "SK", Signal::SKReporter, this);
		break;
	    case 'D':
		CreateAndSetReporter (gno, "DK", Signal::DKReporter, this);
		break;
	}
    }
    if (TStop)
	TStop->ProcessLoadComplete();

    if (interlocked) {
	if (!SetReporterIfExists (gno, "CLK", Signal::CLKReporter, this))
	    if (gno && GetRelay2NoCreate (gno, "CO"))
		MiscG |= SIGF_HARDWIRE_COCLK;
    }
    SetReporterIfExists (gno, "LH", Signal::LunarReporter, this);

    /* use HV/DV if we've got 'em */

    if (!SetReporterIfExists (gno, "HV", Signal::HReporter, this))
	SetReporterIfExists (gno, "H", Signal::HReporter, this);
		
    if (!SetReporterIfExists (gno, "DV", Signal::DReporter, this))
	SetReporterIfExists (gno, "D", Signal::DReporter, this);

    /* If PBS is not defined, search for BPBS; if neither, create PBS */
    if (interlocked && PBS == NULL) {
	ReportingRelay * r = ReportingRelayNoCreate (gno, "PBS");
	if (r == NULL)
	    r = ReportingRelayNoCreate (gno, "BPBS");
	if (r == NULL)
	    r = CreateReportingRelay (intern_rlysym (gno, "PBS"));
	r->SetReporter (Signal::RReporter, this);
	PBS = r;
    }
    SetReporterIfExists (gno, "SH", Signal::LunarWhenRedReporter, this);
    SetReporterIfExists (gno, "STR", Signal::STRReporter, this);
}

void Stop::ProcessLoadComplete () {
    int gno = (int)Sig->CanonicalNumber();
    CreateAndSetReporter (gno, "V", VReporterReporter, this);
    /* now we need NVP/RVP even for autos ... */
    NVP = CreateQuislingRelay (gno, "NVP");
    RVP = CreateQuislingRelay (gno, "RVP");
    NVP->State = 1;
    if (Sig->HomeP())
	VPB = CreateQuislingRelay (gno, "VPB");
}

void TrafficLever::ProcessLoadComplete() {
    RL =  CreateQuislingRelay (XlkgNo, "RL");
    RL->State = 0;
    NL =  CreateQuislingRelay (XlkgNo, "NL");
    NL->State = 0;
    TrafficLeverIndicator * norm = &Indicators[NormalIndex];
    TrafficLeverIndicator * rev = &Indicators[ReverseIndex];
    SetReporterIfExists (XlkgNo, "NFK", TrafficLeverIndicator::WhiteReporter, norm);
    SetReporterIfExists (XlkgNo, "NFKR", TrafficLeverIndicator::RedReporter, norm);
    SetReporterIfExists (XlkgNo, "RFK", TrafficLeverIndicator::WhiteReporter, rev);
    SetReporterIfExists (XlkgNo, "RFKR", TrafficLeverIndicator::RedReporter, rev);
    /* coding for some other day*/
}


void Turnout::ProcessLoadComplete () {
    NWP = CreateQuislingRelay (XlkgNo, "NWP");
    NWP->State = 1;
    RWP = CreateQuislingRelay (XlkgNo, "RWP");
    CreateQuislingRelay (XlkgNo, "NWC")->State = 1;

    CreateAndSetReporter (XlkgNo, "LS", LSReporter, this);

    /* Support both behaviors (buggy NWZ control and real lever sim) */
    NL = CreateQuislingRelay (XlkgNo, "NL");
    RL = CreateQuislingRelay (XlkgNo, "RL");

    NWZ = CreateAndSetReporter(XlkgNo, "NWZ", NWZReporter, this);
    NWZ->State = 1;

    RWZ = CreateAndSetReporter(XlkgNo, "RWZ", RWZReporter, this);
    RWZ->State = 0;
    SetReporterIfExists (XlkgNo, "CLK", CLKReporter, this);
#ifndef XTG
    CreatePointLabels();
#endif
}


void ExitLight::ProcessLoadComplete () {
    CreateAndSetReporter (XlkgNo, "K", KReporter, this);
    XPB = CreateQuislingRelay (XlkgNo, "XPB");
}

void RegisterHelpMenuTextCRLF1 (LPCSTR text, LPCSTR title) {
    size_t l = strlen (text);
    char * buf = new char[2*l+1]; /* it's long -- don't stack allocate */
    const char * p = text;
    char * q = buf;
    while (1) {
	if (*p == '\n')
	    *q++ = '\r';
	*q++ = *p++;
	if (q[-1] == '\0')
	    break;
    }
    RegisterHelpMenuText (buf, title);
    delete [] buf;
}

int ProcessRouteForm (Sexpr s, const char* fname) {
    if (ListLen(s) < 4)
	LERROR ("Should be at least 5 items in ROUTE?", s);

    if (CAR(s).type != L_STRING)
	LERROR ("#2 item in route list not string", CAR(s));
    INameRetval = _strdup (CAR(s).u.s);
    SPop(s);
    if (CAR(s).type != L_CHAR)
	LERROR ("#3 item in route list not char", CAR(s));
    XGlobalRoutid = CAR(s).u.c;
    SPop(s);
    if (CAR(s).type != L_NUM)
	LERROR ("#4 item in route list not number", CAR(s));
#ifndef NXV2
    Station_base = (int)CAR(s).u.n;
#endif
    SPop(s);
    if (CAR(s).type != L_ATOM)
	LERROR ("#5 item in route list not ATOM", CAR(s));
    if (symcmp (CAR(s), "NORTH"))
       Glb.RightIsSouth = FALSE;
    else if (symcmp (CAR(s), "SOUTH"))
       Glb.RightIsSouth = TRUE;
    else LERROR ("Unknown north/south symbol", CAR(s));
    SPop(s);
    for(;CONSP(s) && CONSP(CDR(s)); s = CDDR(s)) {
	Sexpr p = CAR(s);
#ifndef NXV2
	if (symcmp (p, ":SIM-FEET-PER-SCREEN"))
	    ComputeHorizontalScaling(*LCoerceToFloat (CADR(s)).u.f);
	else
#endif
	    if (symcmp (p, ":CRUISING-FEET-PER-SECOND"))
	    CruisingSpeed = *LCoerceToFloat (CADR(s)).u.f;
    	else if (symcmp (p, ":YELLOW-FEET-PER-SECOND"))
	    YellowFeetPerSecond = *LCoerceToFloat (CADR(s)).u.f;
	else if (symcmp (p, ":TRAIN-LENGTH-FEET"))
	    Glb.TrainLengthFeet = *LCoerceToFloat (CADR(s)).u.f;
	else if (symcmp (p, ":TORONTO"))
	    Glb.TorontoStyle = TRUE;
	else if (symcmp (p, ":IRT"))
	    Glb.IRTStyle = TRUE;
	else if (symcmp (p, ":HELP-TEXT")) {
	    Sexpr value = CADR(s);
	    if (!CONSP(value) || !CONSP(CDR(value)))
		LERROR ("Value of :HELP-TEXT is not a list at least two long", value);
	    Sexpr s1 = CAR(value);
	    Sexpr s2 = CAR(CDR(value));
            const char * helpMenuTitle = s1.u.s;
            const char * helpMenuText = s2.u.s;
	    if (s1.type != L_STRING || s2.type != L_STRING)
		LERROR ("Members of :HELP-TEXT not strings.", value);
            std::string temptext;
            std::string pathbuf;
            if (strlen(helpMenuText) > 3 && helpMenuText[0] == '@') {
                const char * fp = include_expand_path(fname, helpMenuText+1, pathbuf);
                FILE* f = fopen(fp, "r");
                if (!f) {
                    LERROR("Cannot open referenced text help file", s2);
                    return 0;
                }
                std::vector<char>b(1000);
                while (true) {
                    size_t haveRead = fread(&b[0], 1, b.size(), f);
                    temptext.append(&b[0], haveRead);
                    if (haveRead < b.size())
                        break;
                }
                fclose(f);
                helpMenuText = temptext.c_str(); // strdup?
            }
	    RegisterHelpMenuTextCRLF1 (helpMenuText, helpMenuTitle);
	}
	else if (symcmp (p, ":BASIC-HELP-FILE-ID")) {
	    Sexpr value = CADR(s);
	    if (!CONSP(value) || !CONSP(CDR(value)))
		LERROR ("Value of :BASIC-HELP-FILE-ID is not a list at least two long", value);
	    Sexpr s1 = CAR(value);
	    Sexpr s2 = CAR(CDR(value));
	    if (s1.type != L_STRING)
                LERROR ("First element of :BASIC-HELP-FILE-ID not a string.", value);
	    if (s2.type != L_NUM)
		LERROR ("Second element of :BASIC-HELP-FILE-ID not a number.", value);
            const char * title = s1.u.const_s;
#ifdef NXSYSMac
            const char * URL = NULL;
            if (CONSP(CDDR (value))) { // Spricht man Lisp hier?
                Sexpr s3 = CAR(CDDR(value));
                if (s3.type != L_STRING)
                    LERROR ("Present, third element (URL) of :BASIC-HELP-FILE-ID not a string.", value);
                URL = s3.u.const_s;
            }
            RegisterHelpURL (title, URL);
#else
	    RegisterInHelpfileMenuItem (title, (int)s2.u.n);
#endif
        }
    }
    return 1;
}


static int TrafficLeverProcessLoadComplete (GraphicObject * g) {
    ((TrafficLever *)g)->ProcessLoadComplete();
    return 0;
}

static int StopProcessLoadComplete (GraphicObject * g) {
    ((Stop *)g)->ProcessLoadComplete();
    return 0;
}

static int ExitLightProcessLoadComplete (GraphicObject * g) {
    ((ExitLight *)g)->ProcessLoadComplete();
    return 0;
}

static int SignalProcessLoadComplete (GraphicObject * g) {
#ifdef XTG
    Signal * sig = ((PanelSignal *)g)->Sig;
#else
    Signal * sig = (Signal *) g;
#endif
    sig->ProcessLoadComplete();
    return 0;
}


#ifdef XTG
/*Version 2 process load complete */
void ProcessLoadComplete () {
    TrackCircuitSystemLoadTimeComplete ();
    MapGraphicObjectsOfType (ID_SIGNAL, SignalProcessLoadComplete);
    MapGraphicObjectsOfType (ID_EXITLIGHT, ExitLightProcessLoadComplete);
    MapGraphicObjectsOfType (ID_STOP, StopProcessLoadComplete);
    MapGraphicObjectsOfType (ID_TRAFFICLEVER, TrafficLeverProcessLoadComplete);
    SwitchesLoadComplete();
}

#else

static int TurnoutProcessLoadComplete (GraphicObject * g) {
    ((Turnout *)g)->ProcessLoadComplete();
    return 0;
}

/* Version 1 process load complete */ 
void ProcessLoadComplete () {
    MapGraphicObjectsOfType (ID_SIGNAL, SignalProcessLoadComplete);
    MapGraphicObjectsOfType (ID_EXITLIGHT, ExitLightProcessLoadComplete);
    MapGraphicObjectsOfType (ID_STOP, StopProcessLoadComplete);
    MapGraphicObjectsOfType (ID_TURNOUT, TurnoutProcessLoadComplete);
    MapGraphicObjectsOfType (ID_TRAFFICLEVER, TrafficLeverProcessLoadComplete);
}
#endif


long compute_track_relayno (int trkno, int secno) {
    if (Glb.IRTStyle)
	return secno*10+trkno;
    else {
	long v = secno;
	if (secno < 10)
	    return v+10*trkno;
	if (secno < 100)
	    return v+100*trkno;
	if (secno < 1000)
	    return v+1000*trkno;
	if (secno < 10000)
	    return v+10000*trkno;
	return v+100000L*trkno;
    }
}

static int ReportAllTLNFunarg (GraphicObject * g) {
    ReportToRelay(((TrafficLever *)g)->NL, TRUE);
    return 0;
}

void ReportAllTrafficLeversNormal () {
    MapGraphicObjectsOfType (ID_TRAFFICLEVER, ReportAllTLNFunarg);
}