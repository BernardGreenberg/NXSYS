//
//  RelayIndex.cpp
//  RelayIndex
//
//  Created by Bernard Greenberg on 1/20/21.
//  Cannibalized Relay Compiler
//  Copyright © 2021 BernardGreenberg. All rights reserved.
//


#include <cstring>
#include <time.h>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>

#include "STLExtensions.h"
#include "argparse.hpp"

#include "lisp.h"
#include "SourceLoc.hpp"

#define RELAYS_PER_LINE 8
#define RELAY_WIDTH 10

typedef Rlysym* RLID;

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::stringstream;

namespace fs = std::filesystem;

std::unordered_map<RLID, std::vector<RLID>> BackRefMap;
std::unordered_map<const char *, Sexpr>LabelMap; //atom strings guaranteed EQ
std::unordered_set<RLID> RelaysReferenced;
std::unordered_set<RLID> RelaysDefined;

static int ReferencesRecorded = 0;

void RC_error (int fatal, const char* s, ...) {
    va_list ap;
    va_start (ap, s);
    vfprintf (stderr, s, ap);
    fprintf(stderr, "\n");
    if (fatal)
        exit(3);
}

bool rsym_sorter (const RLID a, const RLID b) {
    assert(a);
    assert(b);
    if (a->n < b->n)
        return true;
    if (a->n > b->n)
        return false;
    // must be ==
    const char * typa = redeemRlsymId(a->type);
    const char * typb = redeemRlsymId(b->type);
    return strcmp(typa, typb) < 0;
}

std::vector<RLID> OrderUOSet(const std::unordered_set<RLID>& S) {
    std::vector<RLID> V;
    for (RLID r : S)
        V.push_back(r);
    std::sort(V.begin(), V.end(), rsym_sorter);
    return V;  // C++11 "move" semantics!!
}

DEFLSYM(AND);
DEFLSYM(OR);
DEFLSYM(NOT);
DEFLSYM2(T_ATOM,T);
DEFLSYM(LABEL);
DEFLSYM(RELAY);
DEFLSYM(TIMER);

DEFLSYM(INCLUDE);
DEFLSYM(COMMENT);
DEFLSYM2(EVAL_WHEN,EVAL-WHEN);
DEFLSYM(LOAD);

void CompileExpr (Sexpr s, RLID being_defined);

void CompileReference (Sexpr s, RLID being_defined) {
    if (s.type != Lisp::RLYSYM)
        RC_error (3, "Non-Rlysym handed to CompileReference.");
    RLID being_referenced = s.u.r;
    std::vector<RLID>& V = BackRefMap[being_referenced];
    if (being_referenced != being_defined &&
        std::find(V.begin(), V.end(), being_defined) == V.end()) {

        V.push_back(being_defined);
        RelaysReferenced.insert(being_referenced);
        ReferencesRecorded++;
    }
}

void CompileList (Sexpr args, RLID being_defined) {
    for (;CONSP(args);SPop(args))
        CompileExpr (CAR(args), being_defined);
}

void CompileExpr (Sexpr s, RLID being_defined) {
    if (s.type == Lisp::tCONS) {
        Sexpr fn = CAR(s);
        if (fn == NOT) {
            Sexpr ss = CADR (s);
            if (!(ss.type == Lisp::RLYSYM))
                RC_error (1, "No non-atomic-relay NOT's.");
            CompileReference(ss, being_defined);
        }
        else if (fn == AND || fn == OR)
            CompileList (CDR(s), being_defined);
        else if (fn == LABEL) {
            if (CDR(s).type != Lisp::tCONS || CDDR(s).type != Lisp::tCONS)
                RC_error (1, "Bad Format LABEL clause.");
            const Sexpr ltag = CADR(s);
            if (ltag.type != Lisp::ATOM)
                RC_error (1, "Label is not atom.");
            Sexpr exp = CDDR(s);
            CAR(s) = AND;
            CAR(CDR(s)) = NIL;
            CDDR(s) = NIL;  // protect from dealloc
            CompileList (exp, being_defined);
            LabelMap[ltag.u.s] = exp;
        }
        else {
            Sexpr f2 = MaybeExpandMacro (s);
            if (f2 != EOFOBJ) {
                CompileExpr (f2, being_defined);
                dealloc_ncyclic_sexp (f2);
            }
            else
                LispBarf (1, "Unknown form", s);
        }
        return;
    }
    else if (s.type == Lisp::ATOM) {
        if (s == T_ATOM || s == NIL) {
        }
        else if (LabelMap.count(s.u.s)) {
            CompileList (LabelMap[s.u.s], being_defined);
        }
        else
            LispBarf (1, "Unknown LABEL in Relay Xreffer", s);
    }
    else if (s.type == Lisp::RLYSYM) {
        CompileReference(s, being_defined);
    }
    else if (s.type == Lisp::NUM) {
    }
    else
        LispBarf (1, "Unknown form (2) in Relay Xreffer", s);
}

void CompileRelayDef (Sexpr s, fs::path path, long filepos) {
    Sexpr rlysexpr = CAR(s);
    RLID relay_sym = rlysexpr.u.r;
    if (RelaysDefined.count(relay_sym))
        RC_error (1, "Relay already defined: %s", relay_sym->PRep().c_str());
    SourceLoc::RecordRelay(path.string().c_str(), relay_sym->PRep().c_str(), filepos);
    RelaysDefined.insert(relay_sym);
    CompileList (CDR(s), relay_sym);
}

void CompileTimerRelayDef (Sexpr s, fs::path path, long filepos) {
    Sexpr nam = SPopCar(s);
    //long time = CAR(s).u.n;  long time no C
    CAR(s) = nam;            /* compile as 422U */
    CompileRelayDef(s, path, filepos);
}

void close_report(fs::path path, std::ofstream& ofs) {
    auto len = ofs.tellp();
    ofs.close();
    cout << "Wrote " << path.string() << ", " << len << " bytes." << endl;
}

void CleanUpRelaySys () {
    for (auto& lte : LabelMap)
        dealloc_ncyclic_sexp (lte.second);
}

void CompileFile(FILE* f, fs::path path);  // for recursive call for INCLUDE

void CompileTopLevelForm (Sexpr s, fs::path path, long filepos) {
    if (s.type != Lisp::tCONS)
        RC_error (1, "Item definition not a list?");
    if (CAR(s).type != Lisp::ATOM) {
        RC_error (1, "Top-level item doesn't start with atom.");}
    else {
        Sexpr fn = CAR(s);
        Sexpr f2 = MaybeExpandMacro (s);
        if (f2 != EOFOBJ) {
            CompileTopLevelForm (f2, path, filepos);
            dealloc_ncyclic_sexp (f2);
        }
        else if (fn == DEFRMACRO)
            defrmacro (s);
        else if (fn == FORMS) {
            SPop (s);
        forms:
            while (s.type == Lisp::tCONS)
                CompileTopLevelForm (SPopCar(s), path, filepos);
        }
        else if (fn == RELAY)
            CompileRelayDef (CDR(s), path, filepos);
        else if (fn == TIMER)
            CompileTimerRelayDef (CDR(s), path, filepos);
        else if (fn == INCLUDE) {
            fs::path newpath (path);
            newpath.replace_filename(CADR(s).u.s);
            FILE * ff = fopen (newpath.string().c_str(), "rb");
            if (ff == NULL)
                RC_error (1, "Cannot open include file %s", newpath.c_str());
            CompileFile (ff, newpath);
        }
        else if (fn == COMMENT);
        else if (fn == EVAL_WHEN) {
            SPop(s);
            if (s.type == Lisp::tCONS && CAR(s).type == Lisp::tCONS) {
                Sexpr sc = SPopCar(s);
                for (; sc.type == Lisp::tCONS; SPop(sc))
                    if (CAR(sc) == LOAD)
                        goto forms;
            }
        }
        else {
            // Random top-level form
        }
    }
}

void CompileFile (FILE* f, fs::path path) {
    for (;;) {
        SourceLoc::RecordFile(path.string().c_str());
        skip_lisp_file_whitespace(f);
        long sexp_pos = ftell(f);
        Sexpr s = read_sexp (f);
        if (s == EOFOBJ)
            break;
        CompileTopLevelForm (s, path, sexp_pos);
        dealloc_ncyclic_sexp (s);
    }
    SourceLoc::ComputeFileLines(path.string().c_str(), f);
    fclose (f);
}


void CompileLayout (FILE* f, fs::path path) {
    SetLispBarfString ("Relay Xreffer");
    CompileFile (f, path);
}

string padit (string s, long n) {
    long len = (long)s.size();
    if (len < n)
        s += string(n-len, ' ');
    else
        s += ' ';
    return s;
}

void IndexOneRelay(RLID rsym, std::ofstream& outs) {
    auto& references = BackRefMap[rsym];
    outs << endl << rsym->PRep() << " ";
    SourceLoc::Info SLinfo;
    bool known = SourceLoc::getSourceLoc(rsym->PRep().c_str(), SLinfo);
    if (known) {
        outs << SLinfo.line_number << " " << SLinfo.file_pos << " " << SLinfo.file;
    } else {
        outs << "UNDEFINED";
    }
    std::sort(references.begin(), references.end(), rsym_sorter);

    int i = 0;
    for (auto ref : references) {
        if ((i % RELAYS_PER_LINE) == 0)
            outs << endl << " ";
        outs << padit(ref->PRep(), RELAY_WIDTH);
        i++;
    }
    outs << endl;
}

int main (int argc, const char ** argv) {
    
    string compdesc = FormatString("Relay Indexer of %s %s", __DATE__, __TIME__);
#if DEBUG | _DEBUG
    compdesc += " (DEBUG)";
#endif
#ifdef _WIN64
    compdesc += " (64-bit)";
#endif
#ifdef NXSYSMac
     #if defined(__aarch64__) || defined(_M_ARM64)
        compdesc += " (ARM64) ";
      #else
        compdesc += " (Intel) ";
      #endif
#endif

    cout << compdesc << endl;
    
    argparse::ArgSet Aset (compdesc,
                          {
        {"source", "help=Source .trk file (main)."},
        {"-o", "--outpath", "help=Non-default listring path (dft = source.xref}"}});
    
    auto args = Aset.Parse(argc, argv);

    fs::path inpath = args["source"];
    if (!inpath.has_extension())
        inpath.replace_extension(".trk");

    FILE* f = fopen (inpath.string().c_str(), "rb");
    if (f == NULL) {
        cerr << "Cannot open " << inpath << " for reading" <<endl;
        return 3;
    }

    CompileLayout (f, inpath.c_str()); //calls fclose
    SourceLoc::Correlate();
    
    stringstream relay_def_note;
    relay_def_note << RelaysDefined.size() << " relays defined, " << RelaysReferenced.size() << " referenced, " << ReferencesRecorded << " references." << endl;
    cout << relay_def_note.str();
    // will be output to file later

    fs::path outpath = args["outpath"] ? args["outpath"] : fs::path(inpath).replace_extension(".xref");
    fs::path tagspath = fs::path(inpath).replace_filename("TAGS");
    if (!SourceLoc::WriteTagsFile(tagspath.string().c_str())) {
        cerr << "Can't write " << tagspath << "\n";
        return 3;
    }
    cout << "Wrote " << tagspath.string() << ", " << SourceLoc::get_file_size(tagspath.string().c_str()) << " bytes." <<endl;

    std::ofstream outs(outpath);
    if (!outs.is_open()) {
        cerr << "Cannot open output file " << outpath << endl;
        return 2;
    }
    time_t timer = time(NULL);
    struct tm* tblock = localtime (&timer);
    outs << "; RelayXref of " << inpath.string() << " at " << asctime(tblock) << "; by " << compdesc <<endl;
    outs << "; " << relay_def_note.str();

    for (auto rsym : OrderUOSet(RelaysReferenced)) {
        IndexOneRelay(rsym, outs);
    }
    close_report(outpath, outs);
    SourceLoc::Clear(); // QA it for NXSYS
    
    return 0;
};

// I suppose this is right even on Windows.
void MessageBox(void*, const char* msg, const char* title, int) {
    cerr << msg << ": " << title << endl;
}

