#include "windows.h" // messagebox, BOOL
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <vector>
#include <queue>
#include <exception>
#include "MessageBox.h"

#if NXSYSMac
#define CALL_COMPILED 1
#endif

#include "STLExtensions.h"

static void InternalConsoleRelayTracer(const char* name, int state) {
    printf("%-8s  %s\n", name, state ? "PICK" : "DROP");
}


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
static int Halted = 0;

static LNode ONE;
static LNode ZERO;

static bool Running = false;

class NXSYSCompilerException : public std::exception {};


#define NOBJ EOFOBJ

static void CmplrErr(Relay * err_relay, Sexpr s, const char * string, ...) {
    std::string message;
    if (err_relay == nullptr)
	message = "Relay language error in unknown top-level form:\n";
    else {
        message += "Relay language error in definition of relay %s:\n";
        message += err_relay->RelaySym.PRep();
    }
    va_list ap;
    va_start (ap, string);
    message += FormatStringVA(string, ap);
    if (s != NOBJ) {
        message += ": ";
        message += s.PRep();
    }
    MessageBoxS(G_mainwindow, message, "Interpretive Relay Logic Loader", MB_OK | MB_ICONEXCLAMATION);
    throw NXSYSCompilerException();
}

long RelayClicks = 0L;

void CheckRelayDisplay();

const int UpdateQueueSize = 750;

class RelayUpdateQueue {
    int count;
    int take_index;
    int put_index;
    Relay * queue[UpdateQueueSize];
public:
    RelayUpdateQueue() {
        count = take_index = put_index = 0;
    }
    bool empty() {
        return count == 0;
    }
    void reset () {
        count = take_index = put_index = 0;
    }
    void put(Relay * relay) {
        if (count >= UpdateQueueSize)
            NxsysAppAbort (0, "Relay Update Queue Overflow");
        queue[put_index++] = relay;
        if (put_index >= UpdateQueueSize)
            put_index = 0;
        count++;
    }
    Relay * take() {
        Relay * relay = queue[take_index++];
        if (take_index >= UpdateQueueSize)
            take_index = 0;
        count --;
        return relay;
    }
};

DEFLSYM(AND);
DEFLSYM(OR);
DEFLSYM(SWIF);
DEFLSYM(NOT);
DEFLSYM(LABEL);
DEFLSYM2(T_ATOM,T);

static RelayUpdateQueue UpdateQueue;

static bool Trace = false;
static tRelayTracer Tracer = InternalConsoleRelayTracer;

class Label {
public:
    Label (Sexpr s_, LCommShr* v_) : s(s_), v(v_) {}
    Sexpr s;
    LCommShr *v;
};

static std::vector<Label>LabelTable;

static void AddLabel (Sexpr s, LCommShr * v) {
    if (v == NULL) {
	CmplrErr (nullptr, NOBJ, "Null passed to Add Label");
        return; /* placate flow analyzers */
    }
    for (auto& label : LabelTable) {
        if (label.s == s) {
	    if (IgnoreDuplicateRelayLabels)
		return;
            else
		CmplrErr (nullptr, s, "Duplicate label");
        }
    }
    LabelTable.emplace_back(s, v);
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
    while (true) {
        int f = ln->Flags;
        if (f & LF_const)
            return;
        else if (f & LF_Terminal) {
            ((Relay *) ln)->AddDependent(r);
            return;
        }
        else if (f & LF_Not ) {
            ln = ((LNot *) ln)->opd;
        }
        else if (f & LF_Shref) {
            ln = ((LCommShr *) ln)->opd;
        } else {
            Logop * lop = (Logop *) ln;
            for (int i = 0; i < lop->N; i++)
                PropagateDescendent (lop->Opds[i], r);
            return;
        }
    }
}

	
void ReportingRelay::SetReporter (RelRptFcn f, void* obj) {
    ReporterObject = obj;
    ReporterFcn = f;
    Flags |= LF_Reporting;
}

void ReportingRelay::SetNewObject(void*newObj) {
    ReporterObject = newObj;
}

ReportingRelay * CreateReportingRelay (Sexpr s) {
    if (s.u.r->rly != NULL)
	return (ReportingRelay *) (s.u.r->rly);
    else
        return new ReportingRelay(s);
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
    else
	return new ReportingRelay(s);
}

Relay::Relay(Sexpr rlysym) {
    State = 0;
    Flags = LF_Terminal;
    Dependents.clear(); //shouldn't be needed
    exp = &ZERO;
    RelaySym = rlysym;
    rlysym.u.r->rly = this;
}

static BOOL REval (LNode * exp) {
    /*register*/ int f = exp->Flags;
    if (f & LF_Terminal)
	return exp->State;
    if (f & LF_Not)
        return !REval(((LNot *)exp)->opd);
    if (f & LF_Shref)
        return REval(((LCommShr*)exp)->opd);

    Logop * lgp = (Logop*) exp;
    LNode** operands = lgp->Opds;
    switch(lgp->op) {
    case LogOp::ZT:
            return 0;
    case LogOp::NOT:
            return !REval(((LNot *)exp)->opd);
    case LogOp::AND:
            for (int i = 0; i < lgp->N; i++)
                if (!REval(operands[i]))
                    return 0;
            return 1;
    case LogOp::OR:
            for (int i = 0; i < lgp->N; i++)
                if (REval (operands[i]))
                    return 1;
            return 0;
        default:
            return 0; // para placer el compilador
    }
}

BOOL inline Relay::ComputeValue() volatile {
#if CALL_COMPILED && NXCMPOBJ
    if (Flags & LF_CCExp)
        return CallCompiledCode (exp);
#endif

    return REval (exp);
}

bool Relay::maybe_change_state(BOOL new_state) {
    if (State == new_state)
        return false;
    RelayClicks++;
    State = new_state;
    if (Trace)
        Tracer (RelaySym.u.r->PRep().c_str(), State);
    if (Flags & LF_Reporting)
        ((ReportingRelay *)this)->Report();
    UpdateQueue.put(this);
    return true;
}

/*
   The peculiar __attribute__((optnone)) declaration (on the Mac) is there to suppress a seeming
   bug in the clang++ optimizer that I can't narrow down to anything more specific than that without
   it, this function, in a Release build, causes a specific failure under a specific circumstance to be
   described.  It is not at all clear what misapprehension the compiler suffers, and probably not
   worth the time to figure it out.
  
   The bug occurs in (at least) when running the compiled version of Progman St., which works
   perfectly in the Debug (non-optimized) build. Without the optnone attribute, the release and debug builds
   produce very slightly (initially) relay transition traces, enough to cause a major railroad accident,
   i.e., crashes on almost any attempt to set up a route because the locking relays get wrong answers
   (22KXL starts it, I think), then all of the NWZ's start to drop, the *WK's never pick and ... kaboom,
   and relay races and the app crashes with a message. With the optnone, traces are identical and
   the relay-compiled interlocking works perfectly in both builds.
 
   Alternatively, the printf in the code below can be enabled, which seems to derail the optimizer
   enough to make the code work. Or a null function with the same arguments can be substituted
   for it, if declared __attribute__((noinline)), for it seems the activity in the argument
   preparation is the actual derail (as in track device), so to speak.
 
   Note that ComputeValue is inline -- it calls the relay-compiled code in a way that the
   compiler won't understand, but nothing in the relay-compiled code changes any C++ state.
   To reproduce the failure, (comment out the call to printf) and try to complete almost
   any route, e.g, 4 to 14.  The bad relay states can be viewed with Relays | Query Relay.
   Or compile with Trace=true above, invoke NXSYS from a console and capture the standard output.
   There is no data yet on Mac Intel (which I assume would be similar) or Windows compiation
   (because usable relay-compiled objects do not yet exist).

 */
#if NXSYSMac  // we don't know if MSVC has a similar bug; I assume not.
#define DISABLE_OPT_FCN __attribute__((optnone))
#endif

static DISABLE_OPT_FCN void Run (Relay * top_level_relay, BOOL force_new_state) {

    class RunLevelSet {
    public:
        RunLevelSet() {assert(!Running); Running = true;}
        ~RunLevelSet() {Running = false;}
    } setter;

    top_level_relay->maybe_change_state(force_new_state);

    int run_transition_count = 0;
    while (!UpdateQueue.empty() && !Halted) {
        Relay * r = UpdateQueue.take();
        for (auto dependent : r->Dependents) {
#if 0
            assert(dependent);
            assert(dependent->exp);
            printf("Of %s dep %s\n", top_level_relay->RelaySym.PRep().c_str(),
                   dependent->RelaySym.PRep().c_str());
#endif
            if (dependent->maybe_change_state(dependent->ComputeValue()))
                if (++run_transition_count > RCT_MAX)
                    NxsysAppAbort (0, "RELAY RACE! Apparent relay logic instability.");
        }
    }
}



/* Featurette 27 December 1996 - async mouse-ups can try to recurse
   the relay system. */

class DelayQE {
    Relay * relay;
    BOOL    state;
public:
    DelayQE(Relay* r, BOOL s) :
      relay(r), state(s) {}
    void run() {
        Run(relay, state);
    }
};

static std::queue<DelayQE> DelayQueue;

static void EmptyDelayQueue () {
    while (!DelayQueue.empty())
        DelayQueue.pop();
}

static void RunDelayQueue () {
    if (!Running)
        while (!DelayQueue.empty()) {
            DelayQueue.front().run();
            DelayQueue.pop();
	}
    CheckRelayDisplay();
}

static void ExtRun (Relay * r, BOOL state) {
    if (Running)
        DelayQueue.emplace(r, state);
    else
	Run (r, state);
}

void GooseRelay (Relay * rr) {
    int state;
#if defined(CALL_COMPILED) && defined(NXCMPOBJ)
    if (rr->Flags & LF_CCExp) {
//         printf("Goose honking %s\n", rr->RelaySym.PRep().c_str());
        state = CallCompiledCode (rr->exp);
    }

    else
#endif
        state = REval(rr->exp);
    if (rr->State != state)
	Run (rr, state);
    RunDelayQueue();
}

void ReportToRelay (Relay* r, BOOL state) {
    if (r == NULL)
	return;
    if (state == r->State)
	return;
    ExtRun (r, state);
    RunDelayQueue();
}

void ToggleToRelay (Relay* r) {
    if (r == NULL)
	return;
    ExtRun (r, !r->State);
    RunDelayQueue();
}

void PulseToRelay (Relay * r) {
    if (r == NULL)
	return;
    ExtRun (r, 1);
    ExtRun (r, 0);
    RunDelayQueue();
}

static LNode * CompileExpr (Sexpr s, Relay* r);

static LNode * CompileAsAnd (Sexpr s, Relay * r) {
    
    Sexpr cons[2], ns;
    cons[0] = AND;
    cons[1] = s;
    ns.u.l = &cons[0];
    ns.type = Lisp::tCONS;
    return CompileExpr (ns, r);
}


LNode * CompileAsAndTopLevel (Sexpr s, Relay * r) {
    try {
        return CompileAsAnd (s, r);
    } catch (NXSYSCompilerException) {
        return NULL;
    }
}
/* seemingly unused */
#if 0
static LNode * CompileBinOp (Logop_Types op, LNode * exp1, LNode * exp2) {
    Logop pL = *new Logop (op, 2);
    if (pL == NULL)
	memcrash("Binop");
    pL->SetTerm (0, exp1);
    pL->SetTerm (1, exp2);
    return pL;
}
#endif

static LNode * CompileExpr (Sexpr s, Relay* r) {

    if (s.type == Lisp::RLYSYM) {
	if (s.u.r->rly == NULL)
	    s.u.r->rly = CreateRelay(s);
	s.u.r->rly->AddDependent (r);
	return s.u.r->rly;
    }
    else if (s.type == Lisp::tCONS) {
	int n = ListLen (CDR(s));
	LogOp op;
	Sexpr fn = CAR(s);
	if (fn == AND || fn == OR) {
            if (n == 0)
                return (fn == AND) ? &ONE : &ZERO;
	    SPop(s);
	    if (n == 1)
		return CompileExpr (CAR(s), r);
            op = (fn == AND) ? LogOp::AND : LogOp::OR;
	    Logop* pLogop = new Logop (op, n);
            assert (pLogop);
	    for (int x = 0;CONSP(s);SPop(s), x++)
		pLogop->SetTerm (x, CompileExpr(CAR(s), r));
	    return pLogop;
	}
	else if (fn == NOT)
	    /* flush inside nots?  DeMorganize? */
	    return new LNot (CompileExpr (CAR(CDR(s)), r));
	else if (fn == LABEL) {
	    SPop(s);
	    if (s.type != Lisp::tCONS)
		CmplrErr (r, NOBJ, "Bad Format LABEL clause.");
	    if (CDR(s).type != Lisp::tCONS)
                CmplrErr (r, NOBJ, "Bad Format LABEL clause.");
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
	    CmplrErr (r, s, "Unknown form");
	}
						
    }			
    else if (s.type == Lisp::ATOM) {
	if (s == NIL)
	    return &ZERO;
	else if (s == T_ATOM)
	    return &ONE;
	else {
            for (auto& theLabel : LabelTable)
		if (theLabel.s == s) {
		    PropagateDescendent (theLabel.v, r);
		    return theLabel.v;
		}
	}
	CmplrErr (r, s, "Logic label not found");
    }
    else if (s.type == Lisp::NUM) {
	if ((int)s == 1)
	    return &ONE;
	else if ((int)s == 0)
	    return &ZERO;
	else
	    CmplrErr (r, s, "Number found as expression");
    }
    else
	CmplrErr (r, s, "Random object found as expression");
	
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

	ONE.Flags = ZERO.Flags = LF_Terminal | LF_const;
	ONE.State = 1;
	ZERO.State = 0;
	Initsw = 1;
    }
    Halted = 0;
    RunTimers();
    UpdateQueue.reset();
    CreateReportingRelay(intern_rlysym (0, "LOGICHALT"))
	    ->SetReporter (LogicHalter, NULL);
}

class TimerCtl {
public:
    Relay* Ctrler;
    Relay* Outter;
    long StartedTiming;
    int  Interval;
    int  Timing;
    TimerCtl (Relay * outter, Relay* ctrler, int time);
};

static std::vector<std::unique_ptr<TimerCtl>> Timers;

TimerCtl::TimerCtl (Relay * outter, Relay* ctrler, int time) :
    Ctrler(ctrler), Outter(outter), Interval(time), Timing (0) {
        Timers.emplace_back(this); //Construct unique_ptr in place
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
    std::string zname = redeemRlsymId (base.u.r->type);
    zname += "Z";
    return intern_rlysym (base.u.r->n, zname.c_str());
}

Relay* DefineTimerRelayFromLisp (Sexpr s) {
    try {
        if (s.type != Lisp::tCONS)
            CmplrErr (nullptr, NOBJ, "No timer relay name present in TIMER form.");
        Sexpr nam = CAR(s);
        if (nam.type != Lisp::RLYSYM)
             CmplrErr (nullptr, nam, "TIMER relay name not a relay symbol");
        Relay * outter = CreateRelay (nam);
        if (s.type != Lisp::tCONS)
            CmplrErr (outter, NOBJ, "TIMER time and expression absent");
        SPop(s);
        Sexpr TimeNum = CAR(s);
        if (TimeNum.type != Lisp::NUM)
            CmplrErr (outter, TimeNum, "TIMER time (in seconds) not an integer");
        ReportingRelay * ctrler = CreateReportingRelay (ZAppendRlysym (nam));
        TimerCtl * tc = new TimerCtl (outter, ctrler, (int)TimeNum * 1000);
        outter->Flags |= LF_Timer;
        ctrler->SetReporter(TimerRelayFcn, tc);
        ctrler->exp = CompileAsAndTopLevel (CDR(s), ctrler);
        return ctrler->exp ? outter : NULL;
    } catch (NXSYSCompilerException) {
        return NULL;
    }
}

Relay * DefineTimerRelayFromObject (Relay* outter, int t) {
    /* the compiled relay, value referenced by code, is the
       "out(pu)ter".  Its exp, however, is the code for the "controller". */
    ReportingRelay * ctrler = CreateReportingRelay (ZAppendRlysym (outter->RelaySym));
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

Relay * DefineRelayFromLisp2 (Sexpr S, Sexpr exp) {
    try {
        if (S.type != Lisp::RLYSYM)
            CmplrErr (nullptr, S, "Relay name should be a relay symbol, but is not");
        Relay * us = CreateRelay (S);
        LNode * ln = CompileAsAndTopLevel (exp, us);
        if (ln) {
            us->exp = ln;
            return us;
        }
        return NULL;
    }
    catch (NXSYSCompilerException) {
        return NULL;
    }
}

Relay * DefineRelayFromLisp (Sexpr s) {
    return DefineRelayFromLisp2 (CAR(s), CDR(s));
}

Logop::Logop (LogOp t, int n) : LNode() {
    N = n;
    op = t;
    Opds = new LNode *[n];
    assert(Opds);
}

void Relay::AddDependent (Relay * dependent) {
    assert(dependent && "Dependent should not be null");
    for (Relay* maybe : Dependents)
        if (dependent == maybe)
            return;
    Dependents.push_back(dependent);
}

void ValidateRelayWorld();

void Rlysym::DestroyRelayLogic() {
 //   ValidateRelayWorld();
    if (rly == NULL)
        return;
    rly->DestroyLogic();
}

void Relay::DestroyLogic() {
    if (!(Flags & LF_CCExp))
	if (exp != nullptr)
	    DeallocExp(exp);
    exp = nullptr;
    Dependents.clear();
}

void Rlysym::DestroyRelay() {
    ValidateRelayWorld();
    if (rly == NULL)
	return;
    if (!(rly->Flags & LF_Shref))	/* in compiled code linkage sctn */
	delete rly;
    rly = NULL;
}

static void CleanupLabelTableExps() {
    ValidateRelayWorld();
    // Must run backwards!
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
    // Must run backwards!
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
    UpdateQueue.reset();
    Running = false;
    ValidateRelayWorld();
    Timers.clear();  //deletes blocks via unique_ptrs.
    EmptyDelayQueue();
//    ValidateRelayWorld();
    map_relay_syms_method (&Rlysym::DestroyRelayLogic);
    CleanupLabelTableExps();
    CleanupLabelTableShrefs();
    LabelTable.clear();
    map_relay_syms_method (&Rlysym::DestroyRelay);

    Halted = 0;
    Initsw = 0;
 }

int RelayUseDefined (Relay * rr) {
    if (rr == NULL)
	return 0;
    return rr->Dependents.size() > 0;
}

int RelayState (Relay* rr) {
    return rr->State;
}

void SetRelayTrace (tRelayTracer function)  {
    if (function == NULL)
        Trace = false;
    else {
	Trace = true;
	Tracer = function;
    }
}

bool relay_has_exp(Relay* r) {
    return r && r->exp;
}

static char contextBuf[200];

void validateErr(const char * fmt, ...) {
    va_list (ap);
    va_start (ap, fmt);
    char buf[1000];
    vsnprintf(buf, sizeof(buf), fmt, ap);
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
    if (Lopp->op != LogOp::AND && Lopp->op != LogOp::OR) {
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
    snprintf(contextBuf, sizeof(contextBuf), "RObarray slot %d", i);
    if (rsp == NULL){
        return;
    }
    snprintf(contextBuf, sizeof(contextBuf), "RObarray slot %d list item %d", i, j++);
    
    if (!validatePointer(rsp, "relay sym ptr"))
        return;
    long n = rsp->n;
    if (n < 0 || n > 10000000) { // Dave uses magic large #'s
        validateErr("Bad object# in relay: %ld", n);
    } else {
#if 0 //no more "next"!
        char name[48];
        short tpx = rsp->type;
        const char * nam    = redeemRlsymId(tpx);

        sprintf(name, "Relay %ld%s", n, nam);
        strcpy(contextBuf, name);
        if (validateRlysym1(rsp, name)) {
            if (validatePointer(rsp->next, "relaysym next pointer")) {

            }
        }
#endif
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
        if (rly->RelaySym.type != Lisp::RLYSYM) {
            validateErr("Relay Sym ptr is not of type Lisp::RLYSYM but %d", rly->RelaySym.type);
        } else if (rly->RelaySym.u.r != rsp) {
            validateErr("RelaySym ptr %p != %p which latter brought us here.", rly->RelaySym.u.r, rsp);
        } else if (!(rly->Flags & LF_Terminal)) {
            validateErr("Relay terminal bit 0x01 missing in flags 0x%2X", rly->Flags);
        } else if (rly->State != 0 && rly->State != 1) {
            validateErr("Relay lnode state not 0 or 1, but 0x%2X", rly->State);
        }
        else {
            if (rly->Dependents.size() > 300) { // az's etc c/b large
                validateErr("NDependents (%d) not credible.", rly->Dependents.size());
            } else {
                for (auto e : rly->Dependents) {
                    validatePointer(e, "dependent");
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
    return false;
}

void ValidateRelayWorld () {
  //  map_relay_syms_for_validate (ValidateRelaySym);
}
