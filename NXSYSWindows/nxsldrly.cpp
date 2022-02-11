#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lisp.h"
#include "relays.h"
#include "ldraw.h"
#include "nxldapi.h"
#include "compat32.h"
#include "incexppt.h"

static int Initsw = 0;
extern char app_name[];

void DrawCircuit (LNode * relay, LNode* expression, int timer);
LNode * CompileAsAndTopLevel(Sexpr s, Relay * r);
void DeallocExp (LNode *);

/* declare ahead */
static void DrawFile (FILE * f, const char * fname);

LNode ONE;
LNode ZERO;

static Sexpr LABEL, RELAY, TIMER, INCLUDE;
static int GotMagicsyms = 0;

#define RTKLE "Read Track Layout Error"

#define BARF(x) {MessageBox(0,x,RTKLE,MB_OK);return 0;}

Sexpr read_sexp (FILE* f);

static int Totsyms, Symx;
static Relay **Relays;

static void TotalRealRelaySyms (Rlysym *rs, void* vv) {
    if (rs->rly != NULL)
	if (rs->rly->exp != NULL)
	    if ((rs->rly->Flags & LF_CCExp) == 0)
		Totsyms++;
}

static void CollectRelaySyms (Rlysym* rs, void*vv) {
    if (rs->rly != NULL)
	if (rs->rly->exp != NULL)
	    if ((rs->rly->Flags & LF_CCExp) == 0)
		Relays[Symx++] = rs->rly;
}

static int relay_sorter (const void *a, const void *b) {
    Relay * A = *((Relay **) a);
    Relay * B = *((Relay **) b);
    Rlysym *ar = A->RelaySym.u.r;
    Rlysym *br = B->RelaySym.u.r;
    if (ar->n < br->n)
	return -1;
    if (ar->n > br->n)
	return 1;
    if (ar->type != br->type)
	return strcmp (redeemRlsymId (ar->type), redeemRlsymId (br->type));
    return 0;
}

int PrintInterlocking (const char * Iname) {
    Totsyms = 0;
    map_relay_syms (TotalRealRelaySyms);
    if (Totsyms == 0) {
	MessageBox (0, "No Expr-code relays loaded at this interlocking.  "
		    "Run the appropriate .trk file",
		    app_name, MB_OK | MB_ICONSTOP);
	return 0;
    }
    Relays = new Relay *[Totsyms];
    if (Relays == NULL) {
	MessageBox (0, "Can't allocate relay sort array, sorry.",
		    app_name, MB_OK |MB_ICONEXCLAMATION);
	return 0;
    }
    if (!StartPrintRelays (Iname, 1)) {
	delete Relays;
	Relays = NULL;
	return 0;
    }

    Symx = 0;
    map_relay_syms (CollectRelaySyms);
    /* eliminate timer controls */
    for (int iii = 0; iii < Totsyms; iii++) {
	Relay * r = Relays[iii];
	if (r && r->Flags & LF_Timer) {
	    Sexpr controlse = ZAppendRlysym(r->RelaySym);
	    for (int j = 0; j < Totsyms; j++) {
		if (Relays[j] == controlse.u.r->rly)
		    Relays[j] = NULL;
	    }
	}
    }
    int putk = 0;
    for (int ii = 0; ii < Totsyms; ii++) {
	if (Relays[ii] != NULL)
	    if (ii == putk)
		putk++;
	    else
		Relays[putk++] = Relays[ii];
    }
    Totsyms = putk;

    qsort (Relays, Totsyms, sizeof(Relay *), relay_sorter);

    for (int i = 0; i < Totsyms; i++) {
	Relay * r = Relays[i];
	LNode * exp = r->exp;
	/* identify timers */
	int ist = 0;
	if (r && r->Flags & LF_Timer) {
	    Sexpr controlse = ZAppendRlysym(r->RelaySym);
	    ist = 1;
	    exp = controlse.u.r->rly->exp;
	}
	/* draw and place the circuit */
	DrawCircuit (r, exp, ist);
	if (! PlaceRelayDrawing())
	    if (PageFrame()) {
		ClearRelayGraphics();
		PlaceRelayDrawing();	/* better damned fit...18 Nov 1996 */
	    }
	    else
		break;
    }
    delete Relays;
    Relays = NULL;
    FinishPrintRelays();
    return 1;
}


int HackTopLevelForm (Sexpr s, const char * fname) {
    if (s.type != L_CONS)
	LispBarf (1, "Top-level item not a list.", s);
    if (CAR(s).type != L_ATOM)
	LispBarf (1, "Top-level item doesn't start with atom.", s);
    else {
	Sexpr fn = CAR(s);
	Sexpr f2 = MaybeExpandMacro (s);
	if (f2 != EOFOBJ) {
	    HackTopLevelForm (f2, fname);
	    dealloc_ncyclic_sexp (f2);
	}
	else if (fn == DEFRMACRO)
	    defrmacro_maybe_dup (s, 1);	/* maybe FASDUMP macdef, too!? */
	else if (fn == FORMS) {
	    SPop (s);
	    while (s.type == L_CONS) {
		if (!HackTopLevelForm (CAR(s), fname))
		    return 0;
		SPop (s);
	    }
	}
	else if (CAR(s) == RELAY || CAR(s) == TIMER) {
	    int tsw = (CAR(s) == TIMER);
	    int num = 0;
	    SPop(s);
	    Sexpr Rnam = CAR(s);
	    SPop(s);
	    if (tsw) {
		num = CAR(s).u.n;
		SPop(s);
	    }
	    DrawCircuit (CreateRelay (Rnam), CompileAsAndTopLevel(s, NULL), num);
	    if (! PlaceRelayDrawing())
		if (PageFrame()) {
		    ClearRelayGraphics();
		    PlaceRelayDrawing(); /* better damned fit..27 Dec 1996 */
		}
		else
		    return 0;
	}
	else if (CAR(s) == INCLUDE) {
		std::string BUF;
	    const char * ffname = include_expand_path (fname, CADR(s).u.s, BUF);
	    FILE* ff = fopen (ffname, "r");
	    if (ff == NULL) {
		char buf [MAXPATH+50];
		sprintf (buf, "Cannot open %s for reading.", ffname);
		MessageBox (0, buf, RTKLE, MB_OK|MB_ICONEXCLAMATION);
		return 0;
	    }
	    DrawFile (ff, ffname);
	}
    }
    return 1;
}

static void DrawFile (FILE * f, const char * fname) {
    for (;;) {
	Sexpr s = read_sexp (f);
	if (s.type == NULL)
	    break;
	int r = HackTopLevelForm (s, fname);
	dealloc_ncyclic_sexp (s);
	if (!r)
	    break;
    }
    fclose(f);
}

int DrawInterlockingFromFile (const char * Iname, const char * fname) {
    FILE* f = fopen (fname, "r");
    if (f == NULL) {
	char buf [MAXPATH+50];
	sprintf (buf, "Cannot open %s for reading.", fname);
	MessageBox (0, buf, RTKLE, MB_OK|MB_ICONEXCLAMATION);
	return 0;
    }
    if (!GotMagicsyms) {
	LABEL = intern ("LABEL");
	RELAY= intern ("RELAY");
	TIMER = intern ("TIMER");
	INCLUDE = intern ("INCLUDE");
	GotMagicsyms = 1;
    }

    if (!StartPrintRelays (Iname, 1))
	return 0;
    IgnoreDuplicateRelayLabels = 1;
    DrawFile (f, fname);
    IgnoreDuplicateRelayLabels = 0;
    FinishPrintRelays();
    return 1;
}
    
