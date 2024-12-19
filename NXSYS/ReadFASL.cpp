//
//  ReadFASL.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 12/19/24.
//  Copyright © 2024 BernardGreenberg. All rights reserved.
//  Almost all ancient 1994 pre-STL code.
//

#include <stdlib.h>
#include <string.h>
#include <cassert>

#include "windows.h" //so to speak; defines BOOL on mac and other stuff.

#include "relays.h"
#include "lisp.h"

#include <string>
#include <vector>

using PBYTE = unsigned char*;
using std::string, std::vector;
#include "FASL.H"

extern vector<Relay*>ESD;
extern vector<string> FASLAtsyms;
int InterpretTopLevelForm (const char* fname, Sexpr s);
void FatalAppExit(int, const char * text);

short fasl_getw(PBYTE * p) {
    int blow = *(*p)++;
    int bhigh = *(*p)++;
    return (bhigh << 8) | blow;
}

Sexpr FaslForm (PBYTE * pp) {
    PBYTE p = *pp;
    int ctlb, w;
    char * str;
    Sexpr s, last, current;
    ctlb = *p++;
    switch (ctlb) {
        case FASD_EOF:
            s.type = Lisp::tNULL;
            s.u.s = NULL;
            break;
        case FASD_VERSION:
            if (*p++ != 1)
                FatalAppExit (0, "FASD version not 1.");
            s = NIL;
            break;
        case FASD_CHAR:
            s.type = Lisp::CHAR;
            s.u.c = *p++;
            break;
        case FASD_ATSYM:
            s = intern(FASLAtsyms[fasl_getw (&p)].c_str());
            break;
        case FASD_STRING:
            w = fasl_getw(&p);
            s.type = Lisp::STRING;
            str = (char *)malloc (w+1); /* (oo) */
            str[w] = 0;
            memcpy (str, p, w);
            s.u.s = str;
            p += w;
            break;
        case FASD_1BNUM:
            s.type = Lisp::NUM;
            s.u.n = *p++;
            break;
        case FASD_2BNUM:
            s.type = Lisp::NUM;
            s.u.n = fasl_getw (&p);
            break;
        case FASD_LIST:
            w = fasl_getw (&p);
            last = NIL;
            for (; w > 0; w--) {
                current = CONS (FaslForm (&p), NIL);
                if (last == NIL)
                    s = current;
                else
                    CDR(last) = current;
                last = current;
            }
            break;
        case FASD_RLYSYM:
            s = ESD[fasl_getw(&p)]->RelaySym;
            break;
        default:
            FatalAppExit (0, "Non-understood FASL object.");
    }
    *pp = p;
    return s;
}

void ReadFaslForms (PBYTE p) {
    for (;;) {
        Sexpr s = FaslForm (&p);
        if (s.type == Lisp::tNULL)
            return;
        else if (!(s == NIL)) {
            int success = InterpretTopLevelForm (NULL, s);
            dealloc_ncyclic_sexp (s);
            if (!success)
                break;
        }
    }
}
