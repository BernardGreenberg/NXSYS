#ifndef _NX_SYS_RELAYS_H__
#define _NX_SYS_RELAYS_H__

#include "lisp.h"
#include <vector>

typedef void (*RelRptFcn) (BOOL State, void *Obj);


const int LF_Terminal =  0x01;		/* relay */
const int LF_Reporting = 0x02;		/* reporting relay */
const int LF_Not =       0x04;		/* negator */
const int LF_const =     0x08;		/* constant not part of list struc */
const int LF_Shref =     0x10;		/* shared ref - deall explicit */
const int LF_CCExp =     0x20;		/* compiled code subr */
const int LF_Timer =     0x80;		/* could extend this, you know... */

enum class LogOp {ZT, AND, OR, NOT};

class LNode {
public:
    char State;				/* relays only */
    char Flags;
    LNode() {State = Flags = 0;};
};
    

class LNot : public LNode {
public:
    LNode * opd;
    LNot (LNode * n) {opd = n; Flags = LF_Not;};
};

class LCommShr : public LNode {
public:
    LNode * opd;
    int refct;
    LCommShr (LNode * n)  {
        opd = n;
        refct = 1;
        Flags = LF_Shref;};
};

class Logop : public LNode{
public:
    LogOp op;
    short   N;
    LNode **Opds;
    Logop() : Logop(LogOp::ZT, 0) {}
    Logop (LogOp t) : LNode(), op(t), N(0), Opds(nullptr) {}
    Logop (LogOp t, int n);
    void SetTerm (int n, LNode * lt) {Opds[n]=lt;};
};


class Relay : public LNode {
public:
    Sexpr RelaySym;
    std::vector<Relay*>Dependents;
    LNode * exp;

    Relay(Sexpr s);
    void AddDependent(Relay*dependent);
    void DestroyLogic();
    BOOL ComputeValue ();
    bool maybe_change_state(BOOL new_state);
};

class ReportingRelay : public Relay {
public:
    RelRptFcn ReporterFcn;
    void *ReporterObject;

    void SetReporter (RelRptFcn f, void* obj);
    void SetNewObject(void* newObj);
    void Report() {(*ReporterFcn) (State, ReporterObject);};
    ReportingRelay(Sexpr s) : Relay(s), ReporterFcn(nullptr), ReporterObject(nullptr) {}
};

Relay* get_relay_nocreate (long n, const char * str);
extern long RelayClicks;
ReportingRelay * CreateReportingRelay (Sexpr s);
void MoveReporterAssociatedObject(ReportingRelay* relay, void* object);
Relay* CreateRelay (Sexpr s);
int RelayExpDefined (Relay * rr);
int RelayUseDefined (Relay * rr);
Relay* DefineRelayFromLisp (Sexpr s);
Relay* DefineRelayFromLisp2 (Sexpr relay_sym, Sexpr expression);
Relay* DefineTimerRelayFromLisp (Sexpr s);
Relay* DefineTimerRelayFromObject (Relay* r, int time);
Relay* InitRelay (Relay & rr, Sexpr rlysym);
void InitRelaySys();
void GooseRelay (Relay * rr);
void PulseToRelay (void *);
Sexpr ZAppendRlysym (Sexpr base);
extern int IgnoreDuplicateRelayLabels;
std::vector<Relay*> get_relay_array_for_object_number (int nomenclature);


#endif
