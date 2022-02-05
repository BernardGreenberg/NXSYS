#include "windows.h" // messagebox, BOOL
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>
#include <vector>

#ifndef NXSYSMac
#define CALL_COMPILED 1
#endif



/* This is relay steps before the simulator quiesces when a relay
   race is declared. */

const int RCT_MAX = 600;
int IgnoreDuplicateRelayLabels = 0;

#include "nxsysapp.h"
#include "lisp.h"
#include "relays.h"
#include "timers.h"
#include "cccint.h"
#include "rlytrapi.h"

static int Initsw = 0;
static int NTimers = 0;
static int Halted = 0;

static LNode ONE;
static LNode ZERO;

static int RunLevel = 0;
static jmp_buf JumpBuf;

#define NOBJ EOFOBJ
static Relay * ErrRelay;

static void CmplrErr(Sexpr s, const char * string, ...) {
    char lbuf[50];
    char buf[2048];
    va_list ap;
    if (ErrRelay == NULL)
	strcpy (buf, "Relay language error in unknown top-level form:\n");
    else {
	SexpPRep (ErrRelay->RelaySym, lbuf);
	sprintf (buf, "Relay language error in definition of relay %s:\n",
		 lbuf);
    }
    va_start (ap, string);
    vsprintf (buf+strlen(buf), string, ap);
    va_end (ap);
    if (s != NOBJ) {
	strcat (buf, ": ");
	SexpPRep (s, buf+strlen(buf));
    }
    MessageBox(G_mainwindow, buf, "Interpretive Relay Logic Loader",
	       MB_OK | MB_ICONEXCLAMATION);
    longjmp (JumpBuf, 1);
}

long RelayClicks = 0L;

void CheckRelayDisplay();

const int UpdateQueueSize = 750;
static int UpdateQueuePut = 0, UpdateQueueTake = 0,
    UpdateQueueCount = 0;

static Relay *UpdateQueue[UpdateQueueSize];

static Sexpr AND, OR, SWIF, NOT, T_ATOM, LABEL;
static int Trace = 0;
static tRelayTracer Tracer;

typedef struct {
    Sexpr s;
    LCommShr *v;
} Label;

static std::vector<Label>LabelTable;

static void AddLabel (Sexpr s, LCommShr * v) {
    if (v == NULL) {
	CmplrErr (NOBJ, "Null passed to Add Label");
        return; /* placate flow analyzers */
    }
    for (size_t i = 0; i < LabelTable.size(); i++)
        if (LabelTable[i].s == s) {
	    if (IgnoreDuplicateRelayLabels) {
		return;
         }
	    else {
		CmplrErr (s, "Duplicate label");
         }
        }
    Label newLabel;
    newLabel.s = s;
    newLabel.v = v;
    LabelTable.push_back(newLabel);
}

static void memcrash (const char * str) {
    char buf[128];
    sprintf (buf, "Memory allocation for %s fails.", str);
    NxsysAppAbort (0, buf);
}
    
void DeallocExp (LNode * ln) {
    int f = ln->Flags;
    if (f & LF_Terminal)
	return;
    if (f & LF_Shref)
	return;
    if (f & LF_Not) {
	LNot * lnot = (LNot *) ln;
	DeallocExp (lnot->opd);
	delete lnot;
    }
    else {
	Logop * lop = (Logop *) ln;
	for (int i = 0; i < lop->N; i++)
	    DeallocExp (lop->Opds[i]);
	delete lop; // moved to after above 11 June 2003
    }
}

static void PropagateDescendent (LNode * ln, Relay * r) {
top:
    int f = ln->Flags;
    if (f & LF_const);
    else if (f & LF_Terminal)
	((Relay *) ln)->SetDependent(r);
    else if (f & LF_Not ) {
	ln = ((LNot *) ln)->opd;
	goto top;
    }
    else if (f & LF_Shref) {
       	ln = ((LCommShr *) ln)->opd;
        goto top;
    } else {
	Logop * lop = (Logop *) ln;
	for (int i = 0; i < lop->N; i++)
	    PropagateDescendent (lop->Opds[i], r);
    }
}

	
void ReportingRelay::SetReporter (RelRptFcn f, void* obj) {
    ReporterObject = obj;
    ReporterFcn = f;
    Flags |= LF_Reporting;
}

Relay * InitRelay (Relay & rr, Sexpr rlysym) {
    if (&rr == NULL)
	memcrash("Init Relay");
    rr.State = 0;
    rr.Flags = LF_Terminal;
    rr.NDependents = 0;
    rr.NDepArraySize = 0;
    rr.Dependents = 0;
    rr.exp = &ZERO;
    rr.RelaySym = rlysym;
    rlysym.u.r->rly = &rr;
    return &rr;
}

ReportingRelay * CreateReportingRelay (Sexpr s) {
    if (s.u.r->rly != NULL)
	return (ReportingRelay *) (s.u.r->rly);
    else {
	ReportingRelay* rr = (ReportingRelay *)
			     InitRelay (*new ReportingRelay, s);
	rr->ReporterFcn = NULL;
	return (rr);
    }
}

Relay * GetRelay2NoCreate (long n, const char * string) {
    Sexpr rlysym = intern_rlysym_nocreate (n, string);
    if (rlysym == NIL)
	return NULL;
    return rlysym.u.r->rly;		/* could be NULL */
}

Relay * CreateRelay (Sexpr s) {
    if (s.u.r->rly != NULL)
	return (Relay *) (s.u.r->rly);
    else {
	Relay& rr = *new ReportingRelay; /* great */
	return InitRelay(rr, s);
    }
}


static BOOL REval (LNode * exp) {
again:
    /*register*/ int f = exp->Flags;
    if (f & LF_Terminal)
	return exp->State;
    if (f & LF_Not)
	return !REval(((LNot *)exp) ->opd);
    if (f & LF_Shref) {
	exp = ((LCommShr*)exp)->opd;
	goto again;
    }
    Logop * lgp = (Logop*) exp;
    int n = lgp->N;
    LNode** opp = lgp->Opds;
    if (lgp->op == LG_AND) {

	for (int i = 0; i < n; i++)
	    if (!REval (*opp++))
		return 0;
	return 1;
    }
    else {
	for (int i = 0; i < n; i++)
	    if (REval (*opp++))
		return 1;
	return 0;
    }
}

#ifdef __BORLANDC__
#pragma warn -rch
#endif
static void Run (Relay * rr, BOOL state) {
    RunLevel++;
    int Rct = 0;
    int dno = 0, n = 0;
    Relay * r = NULL;
    goto inloop;
    while (UpdateQueueCount > 0 && !Halted) {
	r = UpdateQueue[UpdateQueueTake++];
	if (UpdateQueueTake == UpdateQueueSize)
	    UpdateQueueTake = 0;
	UpdateQueueCount--;
	n = r->NDependents;
	for (dno = 0; dno < n; dno++) {
	    rr = r->Dependents[dno];
#if defined(CALL_COMPILED) && defined(NXCMPOBJ)
	    if (rr->Flags & LF_CCExp)
		state = CallCompiledCode (Compiled_Linkage_Sptr, rr->exp);
	    else

#endif  
             if (rr->exp == NULL)
		state = rr->State;
	    else
		state = REval (rr->exp);
inloop:	    if (state != rr->State) {
		RelayClicks++;
		rr->State = state;
		if (Trace)
		    Tracer (RlysymPRep (rr->RelaySym), state);
		if (rr->Flags & LF_Reporting)
		    ((ReportingRelay *)rr)->Report();
		if (UpdateQueueCount >= UpdateQueueSize)
		    NxsysAppAbort (0, "Relay Update Queue Overflow");
		if (Rct++ > RCT_MAX)
		    NxsysAppAbort (0, "RELAY RACE! Apparent relay logic instability.");
		UpdateQueue[UpdateQueuePut++] = rr;
		if (UpdateQueuePut >= UpdateQueueSize)
		    UpdateQueuePut = 0;
		UpdateQueueCount++;
	    }
	}
    }
    RunLevel--;
}

/* Featurette 27 December 1996 - async mouse-ups can try to recurse
   the relay system. */

struct RelayQE {
    Relay * relay;
    BOOL    state;
    struct RelayQE *next;
};

static RelayQE *RlqFirst = NULL, *RlqLast = NULL;

static void EmptyRelayCleanups () {
    while (RlqFirst != NULL) {
	RelayQE * qe = RlqFirst;
	RlqFirst = qe->next;
	if (RlqFirst == NULL)
	    RlqLast = NULL;
	delete qe;
    }
}

static void CheckRelayCleanups () {
    if (RunLevel == 0)
	while (RlqFirst != NULL) {
	    RelayQE * qe = RlqFirst;
	    Run (qe->relay, qe->state);
	    RlqFirst = qe->next;
	    if (RlqFirst == NULL)
		RlqLast = NULL;
	    delete qe;
	}
    CheckRelayDisplay();
}

static void ExtRun (Relay * r, BOOL state) {
    if (RunLevel == 0)
	Run (r, state);
    else {
	RelayQE *qe = new RelayQE;
	if (qe == NULL) {
	    MessageBox (0, "Async Queue Entry Allocate fails",
			"Relay System", MB_OK | MB_ICONSTOP);
	    return;
	}
	qe->relay = r;
	qe->state = state;
	qe->next = NULL;
	if (RlqFirst == NULL)
	    RlqFirst = qe;
	else
	    RlqLast->next = qe;
	RlqLast = qe;
    }
}
#ifdef __BORLANDC__
#pragma warn +rch
#endif


void GooseRelay (Relay * rr) {
    int state;
#if defined(CALL_COMPILED) && defined(NXCMPOBJ)
    if (rr->Flags & LF_CCExp)
	state = CallCompiledCode (Compiled_Linkage_Sptr, rr->exp);

    else
#endif
        state = REval(rr->exp);
    if (rr->State != state)
	Run (rr, state);
    CheckRelayCleanups();
}

void ReportToRelay (void* vr, BOOL state) {
    if (vr == NULL)
	return;
    Relay * r = (Relay *) vr;
    if (state == r->State)
	return;
    ExtRun (r, state);
    CheckRelayCleanups();
}

void ToggleToRelay (void* vr) {
    if (vr == NULL)
	return;
    ExtRun ((Relay *) vr, !((Relay *) vr)->State);
    CheckRelayCleanups();
}

void PulseToRelay (void * vr) {
    if (vr == NULL)
	return;
    Relay * r = (Relay *) vr;
    ExtRun (r, 1);
    ExtRun (r, 0);
    CheckRelayCleanups();
}

static LNode * CompileExpr (Sexpr s, Relay* r);

static LNode * CompileAsAnd (Sexpr s, Relay * r) {
    
    Sexpr cons[2], ns;
    cons[0] = AND;
    cons[1] = s;
    ns.u.l = &cons[0];
    ns.type = L_CONS;
    return CompileExpr (ns, r);
}


LNode * CompileAsAndTopLevel (Sexpr s, Relay * r) {
    
    ErrRelay = r;
    if (setjmp (JumpBuf) != 0)
	return NULL;
    return CompileAsAnd (s, r);
}
/* seemingly unused

static LNode * CompileBinOp (Logop_Types op, LNode * exp1, LNode * exp2) {
    Logop& L = *new Logop (op, 2);
    if (&L == NULL)
	memcrash("Binop");
    L.SetTerm (0, exp1);
    L.SetTerm (1, exp2);
    return &L;
}
 */

static LNode * CompileExpr (Sexpr s, Relay* r) {

    if (s.type == L_RLYSYM) {
	if (s.u.r->rly == NULL)
	    s.u.r->rly = CreateRelay(s);
	s.u.r->rly->SetDependent (r);
	return s.u.r->rly;
    }
    else if (s.type == L_CONS) {
	int n = ListLen (CDR(s));
	Logop_Types op;
	Sexpr fn = CAR(s);
	if (fn == AND) {
	    if (n == 0)
		return &ONE;
	    op = LG_AND;
many:	    SPop(s);
	    if (n == 1)
		return CompileExpr (CAR(s), r);
	    Logop& L = *new Logop (op, n);
	    if (&L == NULL)
		memcrash("Logop");
	    for (int x = 0;CONSP(s);SPop(s), x++)
		L.SetTerm (x, CompileExpr(CAR(s), r));
	    return &L;
	}
	else if (fn == OR) {
	    if (n == 0)
		return &ZERO;
	    op = LG_OR;
	    goto many;
	}
	else if (fn == NOT)
	    /* flush inside nots?  DeMorganize? */
	    return new LNot (CompileExpr (CAR(CDR(s)), r));
	else if (fn == LABEL) {
	    SPop(s);
	    if (s.type != L_CONS)
badlab:		CmplrErr (NOBJ, "Bad Format LABEL clause.");
	    if (CDR(s).type != L_CONS)
		goto badlab;
	    LCommShr * v = new LCommShr (CompileAsAnd (CDR(s), r));
	    AddLabel (CAR(s), v);
	    return v;
	}
	else {
	    Sexpr mexp = MaybeExpandMacro (s);
	    if (mexp != EOFOBJ) {
		LNode * v = CompileExpr (mexp, r);
		dealloc_ncyclic_sexp (mexp);
		return v;
	    }
	    CmplrErr (s, "Unknown form");
	}
						
    }			
    else if (s.type == L_ATOM) {
	if (s == NIL)
	    return &ZERO;
	else if (s == T_ATOM)
	    return &ONE;
	else {
	    for (size_t i = 0; i < LabelTable.size(); i++) {
                Label& theLabel = LabelTable[i];
		if (theLabel.s == s) {
		    PropagateDescendent (theLabel.v, r);
		    return theLabel.v;
		}
            }
	}
	CmplrErr (s, "Logic label not found");
    }
    else if (s.type == L_NUM) {
	if (s.u.n == 1)
	    return &ONE;
	else if (s.u.n == 0)
	    return &ZERO;
	else
	    CmplrErr (s, "Number found as expression");
    }
    else
	CmplrErr (s, "Random object found as expression");
	
    return NULL;
}

static void LogicHalter (BOOL state, void *) {
    if (state) {
	if (MessageBox (0, "LOGIC HALT RELAY PICKED!"
			"\n\"Yes\" to take breakpoint, \"No\" to continue.",
			PRODUCT_NAME " Relay Simulation",
		    MB_YESNO | MB_ICONSTOP)
	    == IDYES) {
	    HaltTimers();
	    Halted = 1;
	}
    }
}

void InitRelaySys() {
    if (!Initsw) {
	SetLispBarfString (PRODUCT_NAME " Lisp Substrate");
	AND = intern("AND");
	OR = intern ("OR");
	SWIF = intern ("SWIF");
	NOT = intern ("NOT");
	T_ATOM = intern ("T");
	LABEL = intern ("LABEL");
	ONE.Flags = ZERO.Flags = LF_Terminal | LF_const;
	ONE.State = 1;
	ZERO.State = 0;
	Initsw = 1;
    }
    Halted = 0;
    NTimers = 0;
    RunTimers();
    UpdateQueueCount = UpdateQueuePut = UpdateQueueTake = 0;
    CreateReportingRelay(intern_rlysym (0, "LOGICHALT"))
	    ->SetReporter (LogicHalter, NULL);
}

struct _TimerCtl {
    Relay* Ctrler;
    Relay* Outter;
    long StartedTiming;
    int  Interval;
    int  Timing;
    _TimerCtl (Relay * outter, Relay* ctrler, int time);
};


typedef struct _TimerCtl TimerCtl;


static std::vector<TimerCtl*> Timers;

_TimerCtl::_TimerCtl (Relay * outter, Relay* ctrler, int time) :
    Ctrler(ctrler), Outter(outter), Interval(time), Timing (0) {
        Timers.push_back(this);
}

void TimerTimeFcn (void * v) {
    TimerCtl * tc = (TimerCtl*) v;
/* this seems to happen occasionally - don't know why, but better
   to just ignore it. 2 October 1996 */
//    if (GetTickCount () >= tc->StartedTiming + tc->Interval) {
	tc->Timing = 0;
	ReportToRelay (tc->Outter, tc->Ctrler->State);
//    }
//    else NxsysAppAbort (0, "Premature timer firing.");
}

static void TimerRelayFcn (BOOL state, void * v) {
    TimerCtl * tc = (TimerCtl*) v;
    tc->Timing = state;
    if (state) {
	tc->StartedTiming = GetTickCount();
	NXTimer (tc, TimerTimeFcn, tc->Interval);
    }
    else
	ReportToRelay (tc->Outter, FALSE);
}

Sexpr ZAppendRlysym (Sexpr base) {
    char buf[20];
    strcat (strcpy (buf, redeemRlsymId (base.u.r->type)), "Z");
    return intern_rlysym (base.u.r->n, buf);
}

Relay* DefineTimerRelayFromLisp (Sexpr s) {
    ErrRelay = NULL;
    if (setjmp (JumpBuf) != 0)
	return NULL;
    if (s.type != L_CONS)
	CmplrErr (NOBJ, "No timer relay name present in TIMER form.");
    Sexpr nam = CAR(s);
    if (nam.type != L_RLYSYM)
	CmplrErr (nam, "TIMER relay name not a relay symbol");
    Relay * outter = CreateRelay (nam);
    ErrRelay = outter;
    if (s.type != L_CONS)
	CmplrErr (NOBJ, "TIMER time and expression absent");
    SPop(s);
    Sexpr TimeNum = CAR(s);
    if (TimeNum.type != L_NUM)
	CmplrErr (TimeNum, "TIMER time (in seconds) not an integer");
    ReportingRelay * ctrler = CreateReportingRelay (ZAppendRlysym (nam));
    TimerCtl * tc = new TimerCtl (outter, ctrler, (int)TimeNum.u.n * 1000);
    outter->Flags |= LF_Timer;
    ctrler->SetReporter(TimerRelayFcn, tc);
    ctrler->exp = CompileAsAndTopLevel (CDR(s), ctrler);
    return ctrler->exp ? outter : NULL;
}

Relay * DefineTimerRelayFromObject (Relay* outter, int t) {
    /* the compiled relay, value referenced by code, is the
       "out(pu)ter".  Its exp, however, is the code for the "controller". */
    ReportingRelay * ctrler
	    = CreateReportingRelay (ZAppendRlysym (outter->RelaySym));
    TimerCtl * tc = new TimerCtl (outter, ctrler, t*1000);
    ctrler->SetReporter(TimerRelayFcn, tc);
    /* copy the code pointer into the controller. */
    ctrler->exp = outter->exp;
    ctrler->Flags |= LF_CCExp;
    /* Turn off expression in the outter. */
    outter->exp = NULL;
    outter->Flags &= ~LF_CCExp;
    outter->Flags |= LF_Timer;
    return ctrler;
}

Relay * DefineRelayFromLisp2 (Sexpr r, Sexpr exp) {
    ErrRelay = NULL;
    if (setjmp (JumpBuf) != 0)
	return NULL;
    if (r.type != L_RLYSYM)
	CmplrErr (r, "Relay name should be a relay symbol, but is not");
    Relay * us = CreateRelay (r);
    LNode * ln = CompileAsAndTopLevel (exp, us);
    if (ln) {
	 us->exp = ln;
	 return us;
    }
    return NULL;
}

Relay * DefineRelayFromLisp (Sexpr s) {
    return DefineRelayFromLisp2 (CAR(s), CDR(s));
}

Logop::Logop (Logop_Types t, int n) : LNode() {
    N = n;
    op = t;
    Opds = new LNode *[n];
    if (Opds == NULL)
	memcrash ("Logic operands");
}

void Relay::SetDependent (Relay * dependent) {
    if (dependent == NULL)
	return;				/* for graphics system */
    if (NDepArraySize == 0)
	Dependents = new Relay*[NDepArraySize = 25];
    if (Dependents == NULL) {
	memcrash ("dependents array");
        return; /* placate flow analyzers */
    }
    for (int i = 0; i < NDependents; i++)
	if (Dependents[i] == dependent)
	    return;
    if (NDependents >= NDepArraySize) {
	int ndas = 2*NDepArraySize; 
	Relay** nda = new Relay *[ndas];
	if (nda == NULL)
	    memcrash ("dependents array extend");
	for (int i = 0; i < NDependents; i++)
	    nda[i] = Dependents[i];
	delete Dependents;
	Dependents = nda;
	NDepArraySize = ndas;
    }
    Dependents [NDependents++] = dependent;
}

void ValidateRelayWorld();
void RelaySymCleanupExps (void *v) {
 //   ValidateRelayWorld();
    Rlysym *rs = (Rlysym *) v;
    Relay * rly = rs->rly;
    if (rly == NULL)
	return;
    if (!(rly->Flags & LF_CCExp))
	if (rly->exp != NULL) {
	    DeallocExp(rly->exp);
            rly->exp = NULL;
        }
    if (rly->NDependents > 0) {
	delete rly->Dependents;
	rly->Dependents = NULL;
	rly->NDependents = 0;
    }
    
}

void RelaySymCleanupRelays (void *v) {
    ValidateRelayWorld();
    Rlysym *rs = (Rlysym *) v;
    Relay * rly = rs->rly;
    if (rly == NULL)
	return;
    if (!(rly->Flags & LF_Shref))	/* in compiled code linkage sctn */
	delete rly;
    rs->rly = NULL;
}

static void CleanupLabelTableExps() {
    ValidateRelayWorld();
    for (size_t j = 0; j < LabelTable.size(); j++) {
        size_t i = LabelTable.size() - j - 1;
	LCommShr * v = LabelTable[i].v;
#if 0
        printf ("delete1 label %s\n", LabelTable[i].s.u.s);
#endif
	if (!(v->Flags & LF_Shref)) {
	    MessageBox (0, "Non-shref in Share table?", "NXSYS Relay Sys Cleanup", MB_OK);
        } else if (!(v->Flags & LF_Terminal)) {
	    DeallocExp (v->opd);
            v->opd = NULL;
	}
    }
}

static void CleanupLabelTableShrefs() {
    ValidateRelayWorld();
    for (size_t j = 0; j < LabelTable.size(); j++) {
        size_t i = LabelTable.size() - j - 1;
	LCommShr * v = LabelTable[i].v;
#if 0
        printf ("delete2 label %s\n", LabelTable[i].s.u.s);
#endif
	if (!(v->Flags & LF_Shref)) {
	    MessageBox (0, "Non-shref in Share table?", "NXSYS Relay Sys Cleanup", MB_OK);
        } else if (!(v->Flags & LF_Terminal)) {
	    delete v;
            LabelTable[i].v = NULL;  // make believe we were programming a Mac
	}
    }
}


void CleanUpRelaySys () {
    UpdateQueuePut = UpdateQueueTake = UpdateQueueCount = RunLevel = 0;
    ValidateRelayWorld();
    EmptyRelayCleanups();
//    ValidateRelayWorld();
    map_relay_syms (RelaySymCleanupExps);
    CleanupLabelTableExps();
    CleanupLabelTableShrefs();
    LabelTable.clear();
    map_relay_syms (RelaySymCleanupRelays);
    Halted = 0;
    Initsw = 0;
    for (int j = 0; j < NTimers; j++)
		delete Timers[j];  // these are timer relays, not NXTimers.
    Timers.clear();
    NTimers = 0;
}


int RelayExpDefined (Relay * rr) {
    if (rr == 0)
	return 0;
    return rr->exp != NULL;
}

int RelayUseDefined (Relay * rr) {
    if (rr == NULL)
	return 0;
    return rr->NDependents > 0;
}

int RelayState (Relay* rr) {
    return rr->State;
}

void SetRelayTrace (tRelayTracer function)  {
    if (function == NULL)
	Trace = 0;
    else {
	Trace = 1;
	Tracer = function;
    }
}

bool relay_has_exp(Relay* r) {
    return r->exp != NULL;
}

static char contextBuf[200];

void validateErr(const char * fmt, ...) {
    va_list (ap);
    va_start (ap, fmt);
    char buf[1000];
    vsprintf(buf, fmt, ap);
    printf("Validator error: %s: %s\n", contextBuf, buf);
    DebugBreak();
}


bool validatePointer(const void * p, const char* desc) {
#if 0
    //
    if (p == NULL)
        return true;
    unsigned long q = (unsigned long) p;
    if (false
        //((q & 0x0000FC0000000000) != 0x0000600000000000) &
//        ((q & 0x0000000FF0000000) != 0x0000000100000000)
     //     ((q & 0xFFFFFFFF00000000) != 0x0000000100000000)
        ) {
        validateErr ("Bad storage pointer for %s, %p", desc, p);
        return false;
    }
#endif
    return true;
}

bool validateExpr(const LNode* L, int breadth, int depth) {

    if (L->Flags & LF_Terminal) { // a relay.
        return true;
    }

    if (L->Flags & LF_Not) {
        if (L->Flags & ~LF_Not) {
            validateErr("Bogus flag bits on with LF_not");
            return false;
        }
        if (validatePointer(((LNot*)L)->opd, "NOT operand")) {
            return validateExpr(((LNot*)L)->opd, breadth, depth + 1);
        }
        return false;
    }
    if (L->Flags & LF_Shref) {
        if (validatePointer(((LNot*)L)->opd, "Shref operand")) {
            return validateExpr(((LNot*)L)->opd, breadth, depth + 1);
        }
        return false;
    }
    if (L->Flags & LF_CCExp) {
#ifdef NXSYSMac
        validateErr("Compiled code flag on MAC");
        return false;
#else
        return true;  // shouldn't have any on mac, though.
#endif
    }
    if (L->Flags & LF_const) {
        return true; // that means outta here
    }
    /* must be general opd */
    Logop * Lopp = (Logop*)L;
    int N = Lopp->N;
    if (N < 0 || N > 500) {
        validateErr("Log operand count, %d,  negative or > 500", N);
        return false;
    }
    if (Lopp->op != LG_AND && Lopp->op != LG_OR) {
        validateErr("Log operation not AND or OR: #x%2d", Lopp->op);
        return false;
    }
    if (N != 0) {
        if (Lopp->Opds == NULL) {
            validateErr("Operands pointer is null, but opct (%d) > 0.", N);
            return false;
        }
        for (int i = 0; i < N; i++) {
            if (validatePointer(Lopp->Opds[i], "logical operand arg #nn")) {
                if (!validateExpr(Lopp->Opds[i], i, depth + 1))
                    return false;
            }
        }
    }
    
    return true;
    
}


void map_relay_syms_for_validate (void (fcn)(const Rlysym*, int));
bool validateRlysym1(const Rlysym* rsp, const char* name);

void ValidateRelaySym(const Rlysym * rsp, int i) {
    int j = 0;
    sprintf(contextBuf, "RObarray slot %d", i);
    if (rsp == NULL){
        return;
    }
    sprintf(contextBuf, "RObarray slot %d list item %d", i, j++);
    
    if (!validatePointer(rsp, "relay sym ptr"))
        return;
    long n = rsp->n;
    if (n < 0 || n > 10000000) { // Dave uses magic large #'s
        validateErr("Bad object# in relay: %ld", n);
    } else {
        char name[48];
        short tpx = rsp->type;
        const char * nam    = redeemRlsymId(tpx);
        sprintf(name, "Relay %ld%s", n, nam);
        strcpy(contextBuf, name);
        if (validateRlysym1(rsp, name)) {
            if (validatePointer(rsp->next, "relaysym next pointer")) {

            }
        }
    }
    return ;
}

bool validateRlysym1(const Rlysym * rsp, const char * name) {
    strcpy(contextBuf, name);
    if (validatePointer(rsp->rly, "Relay ptr in rlysym")) {
#if 0 // this is ok
        if (rsp->rly == NULL) {
            validateErr("Relay pointer NULL in relaysym");
            return false;
        }
#endif
        if (rsp->rly == NULL)
            return true;
        const Relay* rly = rsp->rly;
        if (rly->RelaySym.type != L_RLYSYM) {
            validateErr("Relay Sym ptr is not of type L_RLYSYM but %d", rly->RelaySym.type);
        } else if (rly->RelaySym.u.r != rsp) {
            validateErr("RelaySym ptr %p != %p which latter brought us here.", rly->RelaySym.u.r, rsp);
        } else if (!(rly->Flags & LF_Terminal)) {
            validateErr("Relay terminal bit 0x01 missing in flags 0x%2X", rly->Flags);
        } else if (rly->State != 0 && rly->State != 1) {
            validateErr("Relay lnode state not 0 or 1, but 0x%2X", rly->State);
        }
        else {
            if (rly->NDependents < 0 || rly->NDependents > 300) { // az's etc c/b large
                validateErr("NDependents (%d) not credible.", rly->NDependents);
            }
            else if (rly->NDepArraySize < rly->NDependents || rly->NDepArraySize > 2000) {
                validateErr("NDepArraySize (%d) > ndeps(%d) or otherwise unbelievable.",
                            rly->NDepArraySize, rly->NDependents);
            } else {
                int N = rly->NDependents;
                if (validatePointer(rly->Dependents, "Dependents array pointer")) {
                    if (N > 0 && rly->Dependents == NULL) {
                        validateErr("ndep is %d but deps ptr is null.", N);
                    } else {
                        for (int i = 0; i < N; i ++) {
                            validatePointer(rly->Dependents[i], "dependent");
                        }
                        if (validatePointer(rly->exp, "Expression pointer.")) {
                            if (rly->exp != NULL) {
                                validateExpr(rly->exp, 0, 0);
                            }
                            return true;
                        }
                    }
                    
                }
                
            }
        }
        
    }
    return false;
}

void ValidateRelayWorld () {
  //  map_relay_syms_for_validate (ValidateRelaySym);
}
