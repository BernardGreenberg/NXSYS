#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <vector>

#include "lisp.h"
#include "rcdcls.h"
#include "FASL.H"

/* stupid arrays changed to STL 26 December 2024, about time ...*/

static std::vector<unsigned char> FasdBuffer;
static std::vector<const char*> AtsymTable;
static bool FasdDid = false;

static void fasd_putb (int j) {
    FasdBuffer.push_back (j & 0xFF);
    FasdDid = true;
}

static void fasd_putw (int j) {
    fasd_putb (j & 0xFF);
    fasd_putb (j >> 8);
}
    
void fasd_init () {
    FasdBuffer.clear();
    AtsymTable.clear();
    fasd_putb (FASD_VERSION);
    fasd_putb (1);
    FasdDid = false;
}

void fasd_finish() {
    if (FasdDid)
	fasd_putb (FASD_EOF);
}

static int FasdAtsymLookup (Sexpr s) {
    /* remember, the "Lisp reader" interns STRINGS, too.*/
    for (size_t j = 0; j < AtsymTable.size(); j++)
	if (s.u.a == AtsymTable[j])
	    return (int)j;
    /* else, make new entry */
    AtsymTable.push_back(s.u.a);
    return (int)AtsymTable.size() - 1;
}


static void fasd_putmb(uint64_t v, int n) {
    for (int j = 0; j < n; j++) {
        fasd_putb(v & 0xFF);
        v >>= 8;
    }
}

void fasd_form (Sexpr s) {
    switch (s.type) {
        case Lisp::NUM:
	    if (s.u.n >= 0 && s.u.n < 256) {
		fasd_putb (FASD_1BNUM);
		fasd_putb ((short) (s.u.n));
	    }
            else if (s.u.n >= -65536L && s.u.n <= 65535L) {
                fasd_putb (FASD_2BNUM);
		fasd_putw ((short)(s.u.n));
	    }
            else if (s.u.n >= -(1LL<<32) && s.u.n <=1L<<31) {
                fasd_putb (FASD_4BNUM);
                fasd_putmb((uint64_t)s.u.n, 4);
            }
            else {
                fasd_putb (FASD_8BNUM);
                fasd_putmb((uint64_t)s.u.n, 8);
            }
            break;
        case Lisp::STRING:
	    fasd_putb (FASD_STRING);
	    {
		int i = (int)strlen (s.u.s);
		fasd_putw (i);
		for (int j = 0; j < i; j++)
		    fasd_putb (s.u.s[j]);
	    }
	    break;
        case Lisp::RLYSYM:
	    fasd_putb (FASD_RLYSYM);
	    fasd_putw (RelayId(s));
	    break;
        case Lisp::ATOM:
	    fasd_putb (FASD_ATSYM);
	    fasd_putw (FasdAtsymLookup (s));
	    break;
        case Lisp::tCONS:
	    fasd_putb (FASD_LIST);
	    fasd_putw (ListLen (s));
	    for (;;s = CDR(s)) {
		fasd_form (CAR(s));
		if (CDR(s).type != Lisp::tCONS)
		    break;
	    }
	    if (!(CDR(s) == NIL))
		RC_error (1, "Can't fasdump dotted pairs yet.");
	    break;
        case Lisp::CHAR:
	    fasd_putb (FASD_CHAR);
	    fasd_putb (s.u.c);
	    break;
	default:
	    RC_error (1, "Can't fasdump type %d objects yet.", s.type);
    }
}

/* external APIs */
unsigned char * fasd_data (int & count) {
    if (!FasdDid) {
        count = 0;
        return NULL;
    }
    count = (int)FasdBuffer.size();
    return FasdBuffer.data();
}

const char** fasd_atsym_data (int &fasd_atsym_count) {
    fasd_atsym_count = (int)AtsymTable.size();
    return AtsymTable.data();
}

