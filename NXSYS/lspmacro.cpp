#include "lisp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vector>

struct Macro {
    Sexpr sym;
    Sexpr exp;
    int  argct;
};


static std::vector<Macro> Macros;

const int MaxNMacargs = 10;
static Sexpr Macargs[MaxNMacargs];
static int NMacargs;
static Sexpr MacName;

static Sexpr lcopy (Sexpr x) {
    Sexpr copy;
    Sexpr last = NIL;
    while (x.type == Lisp::tCONS) {
	Sexpr lcons = CONS (lcopy (CAR(x)), NIL);
	if (last == NIL)
	    copy = lcons;
	else
	    CDR(last) = lcons;
	last = lcons;
	SPop(x);
    }
    if (last == NIL)
	return x;
    else
	return copy;
}

int defrmacro (Sexpr arg) {
    return defrmacro_maybe_dup (arg, 1);
}

int defrmacro_maybe_dup (Sexpr arg, int ignore_dup) {
    if (!(arg.type == Lisp::tCONS && (CAR(arg) == DEFRMACRO)))
	return 0;
    Sexpr rest = CDR(arg);
    if (rest.type != Lisp::tCONS)
	goto bs;
    if (CAR(rest).type != Lisp::ATOM) {
bs:	LispBarf (0, "Bad Syntax in DEFRMACRO");
	return 0;
    }
    Sexpr sym = CAR(rest);
    SPop(rest);
    if (rest.type != Lisp::tCONS)
	goto bs;
    if (CAR(rest).type != Lisp::NUM)
	goto bs;
    int argct = (int)CAR(rest).u.n;
    SPop(rest);
    if (rest.type != Lisp::tCONS)
	goto bs;
    if (argct < 1 || argct > 10)
	goto bs;
    for (size_t i = 0; i < Macros.size(); i++)
	if (Macros[i].sym == sym) {
	    if (ignore_dup)
		return -1;
	    LispBarf (1, "Duplicate Macro", sym);
	    return 0;
	}
    Macro tempmac;
    tempmac.sym = sym;
    tempmac.argct = argct;
    tempmac.exp = lcopy (CAR (rest));
    Macros.push_back(tempmac);
    return 1;
}

static Sexpr get_macarg_n (int n) {
    if (n < 1 || n > NMacargs) {
	Sexpr b;
	b.type = Lisp::NUM;
	b.u.n = n;
	LispBarf (2, "Macro arg designator out of range", MacName, b);
	return EOFOBJ;
    }
    return lcopy (Macargs [n-1]);
}

static Sexpr get_macarg (Sexpr ns) {
    if (ns.type != Lisp::NUM) {
	LispBarf (1, "Invalid macro arg designator.", ns);
	return EOFOBJ;
    }
    return get_macarg_n((int)ns.u.n);
}
    
static Sexpr macsubst (Sexpr x)  {
    if (x.type != Lisp::RLYSYM)
	return x;
    int n = (int)x.u.r->n;
    if (n == 0)				/* Allow 0 as global */
	return x;
    Sexpr actual = get_macarg_n (n);
    if (actual.type == Lisp::NUM)
	return intern_rlysym (actual.u.n, redeemRlsymId (x.u.r->type));
    if (actual.type == Lisp::RLYSYM)
	return intern_rlysym (actual.u.r->n, redeemRlsymId (x.u.r->type));
    else {
	LispBarf (2, "Invalid actual parameter for relay sym substitution", MacName, actual);
	return x;
    }
}

static Sexpr macexp (Sexpr x) {
    Sexpr copy;
    Sexpr last = NIL;
    if (x.type == Lisp::tCONS && CAR(x) == ARG)
	return (get_macarg (CAR(CDR(x))));  
    while (x.type == Lisp::tCONS) {
	Sexpr lcons = CONS (macexp (CAR(x)), NIL);
	if (last == NIL)
	    copy = lcons;
	else
	    CDR(last) = lcons;
	last = lcons;
	SPop(x);
    }
    if (last == NIL)
	return macsubst(x);
    else
	return copy;
}

Sexpr MaybeExpandMacro (Sexpr s) {
    Sexpr ss = s;
    if (s.type != Lisp::tCONS)
	return EOFOBJ;
    if (CAR(s).type != Lisp::ATOM)
	return EOFOBJ;
    Macro * mp = NULL;
    for (size_t i = 0; i < Macros.size(); i++)
	if (Macros[i].sym == CAR(s)) {
            mp = &Macros[i];
            break;
        }

    if (mp == NULL) {
	return EOFOBJ;
    }
    MacName = CAR(s);
    SPop(s);
    for (NMacargs = 0; NMacargs < MaxNMacargs; NMacargs++) {
	if (s.type != Lisp::tCONS)
	    break;
	Macargs[NMacargs] = CAR(s);
	SPop(s);
    }
    if (NMacargs >= MaxNMacargs) {
	LispBarf (1, "Macro arg overflow", MacName);
	return EOFOBJ;
    }
    Sexpr reslt = EOFOBJ;
    if (NMacargs != mp->argct)
	LispBarf (1, "Wrong number of macro args", ss);
    else reslt = macexp (mp->exp);
    NMacargs = 0;
    MacName = EOFOBJ;
    return reslt;
}

void MacroCleanup() {
    for (size_t i = 0; i < Macros.size(); i++)
	dealloc_ncyclic_sexp (Macros[i].exp);
    Macros.clear();
}
