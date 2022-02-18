#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include "lisp.h"
#include "rcdcls.h"
#include "FASL.H"

#ifdef _WIN32
const int FasdBufSize = 32000;
#else
const int FasdBufSize = 8000;
#endif

static int FasdCount = 0;
static unsigned char FasdBuffer[FasdBufSize];

const int AtsymArraySize = 75;
static int AtsymArrayCount = 0;
static const char *AtsymArray[AtsymArraySize];
static int FasdDid = 0;


static void fasd_putb (int j) {
    if (FasdCount >= FasdBufSize)
	RC_error (1, "Relay compiler Fasd buffer overflow.\n");
    FasdBuffer[FasdCount++] = j & 0xFF;
    FasdDid = 1;
}

static void fasd_putw (int j) {
    fasd_putb (j & 0xFF);
    fasd_putb (j >> 8);
}
    
void fasd_init () {
    FasdCount = 0;
    AtsymArrayCount = 0;
    fasd_putb (FASD_VERSION);
    fasd_putb (1);
    FasdDid = 0;
}

void fasd_finish() {
    if (FasdDid)
	fasd_putb (FASD_EOF);
}

unsigned char * fasd_data (int & count) {
    if (!FasdDid) {
	count = 0;
	return NULL;
    }
    count = FasdCount;
    return FasdBuffer;
}

const char** fasd_atsym_data (int &fasd_atsym_count) {
    fasd_atsym_count = AtsymArrayCount;
    return AtsymArray;
}

static int FasdAtsymLookup (Sexpr s) {
    for (int j = 0; j < AtsymArrayCount; j++)
	if (s.u.a == AtsymArray[j])
	    return j;
    if (AtsymArrayCount >= AtsymArraySize)
	RC_error (1, "Fasdump Atomic Symbol Heap meltdown.");
    int x = AtsymArrayCount++;
    AtsymArray[x] = s.u.a;
    return x;
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
	    else RC_error (1, "Long nums not fasdumped yet.");
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
