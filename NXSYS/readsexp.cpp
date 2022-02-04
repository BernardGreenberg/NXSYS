/* "dam" is the venerable Dave Moon, who hacked this pgm in Sept 1996
    to port my music player to the Macintosh.  -bsg 2 October 1996 */

//---dam: Added Macintosh conditionalization

/* Modernized to C++11 containers, 15 Aug 2019 BSG, relay stuff mainly
   moved out to RelayLispSubstrate.cpp */

#include "oscond.h"

#include <stdlib.h>

#if _UNICODE
#include <tchar.h>
#endif

#if _BLISP
#define GOOD_SYM_DELIMS "&+-_@$%*=:<>!?"
#else
#define _RELAYS 1
#define GOOD_SYM_DELIMS "&+-_@$%*=:"
#endif

#if TEST
#define DOS 1
#endif

#if DOS
#define LispCrash() exit(1);
#else

//---dam: Added Macintosh conditionalization
#if MAC_OS
//#include <Types.h>
void LispCrash() {exit(3);}
#define MessageBoxA MessageBox
#else
#include <windows.h>

#define LispCrash() FatalAppExitA(0, "App Termination ordered by Lisp.");
#endif
#endif

#include "lisp.h"
#include <string.h>
//---dam: Added nonstandard string function declarations
#ifdef MAC_OS
int MessageBox(void*, const char *, const char *, int);
//#include "xstring.h"
#endif //MAC_OS

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include "MapperThunker.h"
#include "RelayLispSubstrate.h"
#include "STLExtensions.h"
#include <ctype.h>


#if ZETALISP
#define ESC_CHAR '/'
#define FRACTION_BAR '\\'
#else
#define ESC_CHAR '\\'
#define FRACTION_BAR '/'
#endif

static const std::string ESC_CHARs(1, ESC_CHAR);
static const std::string FRACTION_BARs(1, FRACTION_BAR);

#if _BLISP
#include "lispmath.h"
#define LISP_ERRORS_TO_CIO 1
#define MB_ICONEXCLAMATION 0
#define MB_ICONSTOP 0
#define MB_OK 0

#else
#include "MessageBox.h"
#endif

#if LISP_ERRORS_TO_CIO
#define MessageBoxA(a,b,c,d) fprintf(stderr, "%s:  %s\n", c, b)
#endif



static const char * LispBarfString = "Lisp Substrate";

void MacroCleanup();

int LispInputSource::Peek () {
    int ch = Getc();
    if (ch != EOF)
	Ungetc(ch);
    return ch;
}

class LispFileInputSource : public LispInputSource {

    private:
	FILE * f;
    public:
	LispFileInputSource (FILE * f_) : f(f_) {};
	int virtual Getc();
	void virtual Ungetc(int c);
	long virtual Tell();
};


int LispFileInputSource::Getc() { return getc(f);}
void LispFileInputSource::Ungetc(int c) { ungetc(c, f);}
long LispFileInputSource::Tell () {return ftell(f);}

class LispStringInputSource : public LispInputSource {

    private:
	LispTChar * s;
    protected:
	int i;
    public:
	LispStringInputSource (LispTChar  * s_)
		: s(s_), i(0), LispInputSource() {IsUnicode = 1;};
	LispStringInputSource () : i(0) {IsUnicode=1;};
	int virtual Getc();

	void virtual Ungetc(int c);
	long virtual Tell();
	int GetIndex () {return i;};
};

class LispNarrowStringInputSource : public LispStringInputSource {
    private:
	char * s;
    public:
	LispNarrowStringInputSource (char  * s_) : s(s_) {};
	int virtual Getc();
};

int LispStringInputSource::Getc() {
    if ( s[i] == '\0')
	return EOF;
#ifndef _UNICODE
    else if (s[i] & 0xFF00){
	LispBarf ("Non-ASCII character in read-from-string string.");
	return EOF;
    }
#endif
    else return s[i++];
}

int LispNarrowStringInputSource::Getc () {
    if ( s[i] == '\0')
	return EOF;
    else return s[i++];
}

void LispStringInputSource::Ungetc(int c) {
    if (i > 0)
	i--;
}

long LispStringInputSource::Tell () {return i;};



void SetLispBarfString (const char * s) {
    LispBarfString = s;
}

// No more! Now std::unordered_map C++11 15 Aug 2019
// const int HASHCT = 401; /* changed from 137 28 January 1998 */


/* This is a hash table of strings tested by "equal" to strings testable by "EQ",
 used to intern "atoms" by EQ. std::unordered_set.emplace basically IS Lisp intern. */
static std::unordered_set<std::string> AtomMap;


static class GoodSymCharInitter {    //The old static-init once-run technique...
    bool array [256];
public:
    GoodSymCharInitter() {
        for (int i = 0; i < 256; i++)
            array[i] = (bool)isalnum(i);
        for (char ch : GOOD_SYM_DELIMS)
            array[ch] = true;
        array[FRACTION_BAR] = true;
    }
    bool operator[](int i) {return array[i];}
} Goodsymchar;

/*
 These symbols are global, and declared in lisp.h.  "intern" operates properly with the STL maps,
 sets, and vectors in this file declared ABOVE this point without any additional initialization.
 You can't do similar in any other file/module, because those STL objects in THIS file might or
 might not have yet been initialized, or will get re-initialized silently when executed in order
 in THIS file -- the generic "static initialization order fiasco" of internet lore.
 
 Other files can, however, use the struct self_snapping_Sexpref in lisp.h to create objects
 that intern the first time they are dereferenced (as Sexpr's) and cache the result, which
 means that the Lisp system can never be reinitialized.  Right now, no initialization other
 than static initializers in this file, including these global vars, is needed or exists.
 SI: is an old Lisp Machine package prefix ("system internals").

 Relays, however, and their tables and maps may be discarded at will, and their system
 reinitialized (e.g., by scenario load/unload).  No static cached pointers to them.

 -- 11/23/2020
 */
Sexpr
symBACKQUOTE = intern ("SI:BACKQUOTE"),
symCOMMA = intern ("SI:COMMA"),
symCOMMAATSIGN = intern ("SI:COMMAATSIGN"),
QUOTE = intern ("QUOTE"),
NOT = intern ("NOT"),
NIL = intern ("NIL"),
symFUNCTION = intern("FUNCTION"),
FORMS = intern ("FORMS"),
DEFRMACRO = intern ("DEFRMACRO"),
ARG = intern ("ARG");

Sexpr
EOFOBJ = Sexpr(L_NULL, "EOF"),
READ_ERROR_OBJ = Sexpr(L_NULL, "READ_ERROR");

LispCB1 LispReadTimeEvalHook = NULL;
LispCB1 LispLoadTimeEvalHook = NULL;

int ListLen (Sexpr s) {
	//---dam: Code Warrior warns for a null statement as a for body, so
	//        I changed it to an empty block
    int l = 0;
    for (l = 0; CONSP(s); SPop(s), l++) {
        assert(l < 5000);
    }
    return l;
}

Sexpr Lisp_Cons (Sexpr s1, Sexpr s2) {
    Sexpr nc;
    nc.u.l = new Sexpr[2];
    if (nc.u.l == NULL) {
	LispBarf("Lisp Cons alloc fails.");
	LispCrash();
    }
    nc.type = L_CONS;
    CAR(nc) = s1;
    CDR(nc) = s2;
    return nc;
}

Sexpr CreateAtom(const char *);

/* We can intern all strings.  No Lisp strings will ever be
   created during running, or that need to be garbage-collected.
   They are thus basically the same as atoms, with a different type.
*/
static const char * intern_string(const std::string& s) {
    //unordered_map emplace does it all, searching, creating or
    //not, and returning an iterator to it!
    return AtomMap.emplace(s).first->c_str();
}

Sexpr intern (const LispTChar * s) {
    return CreateAtom(intern_string(stoupper(s)));
}


static int Stack_count;
static Sexpr Stack[200];

#if _UNICODE
#define I256p(x) ((x & 0xFF00) == 0)
#else
#define I256p(x) (1)
#endif

static int skip_whitespace (LispInputSource&f, int ch) {
top:
    while (I256p (ch) && isspace (ch))
	ch = f.Getc();
        if (ch == ';') {
            do {
                ch = f.Getc();
            } while (!(ch == EOF || ch == '\n'));
            goto top;
    }
    return ch;
}

static Sexpr read_sexp_i (LispInputSource& f, int lf) {
    static LispTChar SymBuf [256];
    static char buf[256];
    Sexpr v1, Last_Cons, First_Cons;
    LispTChar ch2;
    int stack_base = Stack_count;
    short listing = 0, vectoring = 0, rlyf = 0, notf = 0, rmacing = 0;;
    long num = 0; // placate compiler
    int oc = 0;
    int cct;
    int sign;
    int ch = skip_whitespace (f, ' ');
    if (lf) {
	switch (lf) {
	    case 1:
		vectoring =1;
		break;
	    case 5:
		rmacing = 1;
		break;
	    default:
		listing = 1;
		break;
	}
	goto more_lf;
    }
    if (ch == EOF) {
	v1 = EOFOBJ;
retv1:	Stack_count = stack_base;
	return v1;
    }
#if _UNICODE
    if (!I256p (ch))
	goto icd;
#endif
    if (ch == '[') {
	vectoring = 1;
	ch = skip_whitespace (f, ' ');
    }
    else if (ch == '(') {
	listing = 1;
	ch = skip_whitespace (f, ' ');
    }
more_lf:
    if (ch == '[') {
	v1 = read_sexp_i (f, 1);
	ch = ' ';
    }
    else if (ch == '(') {
	v1 = read_sexp_i (f, 2);
	ch = ' ';
    }
    else if (ch == '+' || ch == '-') {
	sign = (ch == '+') ? 1 : -1;
	ch2 = f.Getc();
	if (isdigit (ch2)) {		/* it's a number */
	    cct = 0;
	    SymBuf[cct++] = ch;
	    ch = ch2;
	    goto colnum;
	}
	f.Ungetc(ch2);			/* it's a symbol */
	cct = 0;
	goto colsym;
    }
    else if (isdigit (ch)) {
	sign = 1;
	cct = 0;
colnum:
	for (num = 0; isdigit (ch); ch = f.Getc()) {
	    if (cct < sizeof(SymBuf)/sizeof(SymBuf[0])-1)
		SymBuf[cct++] = ch;
	    num = num * 10 + ch - '0';
	}
#if ! _RELAYS
	if (Goodsymchar[ch] && ch != FRACTION_BAR)
	    goto colsym;
#endif
	if (ch == FRACTION_BAR) {
	    int numerator = (int)num;
	    ch = f.Getc();
	    for (num = 0; isdigit (ch); ch = f.Getc())
		num = num * 10 + ch - '0';
	    if (num == 0) {
		LispBarf("Zero denominator in rational fraction.");
		return NIL;
	    }
#if _BLISP && REDUCED_RATIONALS
	    v1 = CreateReducedRational (sign*numerator, num);
#else
	    v1 = CreateRational (sign*numerator, (int)num);
#endif
	    goto ungnumch;
	}
	else  if (ch == '.')
	    goto col_flonum_got_num;
	else {
	    if (!(!I256p(ch) || isspace (ch) || ispunct (ch) || ch == EOF || ch == ';')) {
		if (Goodsymchar[ch]) {
		    rlyf = 1;
		    goto more_lf;
		}
		sprintf (buf, "Junk after number in SEXP: %c", ch);
		LispBarf (buf);
	    }
	}
	v1.type = L_NUM;
	v1.u.n = num*sign;
#if TRACE_READ
	printf ("Collected fixnum %d\n", num);
#endif
ungnumch:
	if (ch != EOF)
	    f.Ungetc(ch);
	ch = ' ';
    }
    else if (ch == '#') {
	ch = f.Getc();
	if (ch == '.') {
	    Sexpr s = read_sexp_i(f, 0);
	    if (LispReadTimeEvalHook == NULL) {
		LispBarf ("Ignoring Load Time Eval, subbing NIL - #.", s);
		dealloc_ncyclic_sexp(s);
		v1= NIL;
	    }
	    else {
		v1 = (*LispReadTimeEvalHook)(s);
		goto zz;
	    }
	}
	if (ch == '\'') {
	    v1 = CONS (symFUNCTION, CONS (read_sexp_i(f, 5), NIL));
	    goto zz;
	}
	else if (ch == '|') {
	    ch = ' ';
	    do {
		while (ch != EOF && ch != '|') ch = f.Getc();
		if (ch == EOF) {
pdbareof:		LispBarf ("End of file in middle of #| ... |#");
			goto reterr;
		}
		ch = f.Getc();
		if (ch == EOF)
		    goto pdbareof;
	    } while (ch != '#');
	    ch = skip_whitespace(f, ' ');
	    goto more_lf;
	}
	else if (ch == ESC_CHAR) {
	    v1.u.n = 0;
	    v1.u.c = f.Getc();
	    v1.type = L_CHAR;
zz:
	    ch = ' ';
	}
	else {
	    sprintf (buf, "Non-known # escape: %c (0x%X)", ch, ch);
	    LispBarf (buf);
	}
    }
    else if (ch == ESC_CHAR) {
	ch = f.Getc();
	goto force_sym_c;
    }
    else if (ch == ':') {

	while (1) {
	    ch2 = f.Getc();
	    if (ch2 != ' ') {
		f.Ungetc(ch2);
		break;
	    }
	}
	cct = 0;
	goto colsym;
    }
    else if (Goodsymchar[ch]) {
	cct = 0;
colsym:
	for (;Goodsymchar[ch];ch = f.Getc()) {
	    if (cct > sizeof(SymBuf)/sizeof(SymBuf[0])-1) {
		LispBarf("Symbol buffer overflow (256 chars max)");
		return NIL;
	    }
	    SymBuf[cct++] = ch;
	}
	SymBuf[cct] = '\0';
	if (rmacing)
	    f.Ungetc(ch);
	if (rlyf) {
	    rlyf = 0;
#if _BLISP
	    LispBarf("Invalid symbol (numbers followed by letters");
	    return NIL;
#else
            for (char* cp = SymBuf; *cp == 0; cp++)
                *cp = toupper(*cp);
	    v1 = intern_rlysym (num, SymBuf);
#endif
	}
	else
#if _UNICODE
	    v1 = TinternUC (SymBuf);
#else
	    v1 = intern (SymBuf);
#endif
#if TRACE_READ
	printf ("Collected sym %s\n", buf);
#endif
    }
    else if (ch == ESC_CHAR) {
	ch = f.Getc();
force_sym_c:
	cct = 0;
	buf[cct++] = ch;
	ch = f.Getc();
	goto colsym;
    }
    else if (ch == '"') {
        std::string CB;
	//int ucol = (int)f.IsUnicode;
#if _UNICODE
        std::wstring TCB;
#endif
	for (ch = f.Getc();ch != '"'; ch = f.Getc()) {
	    if (ch == EOF) {
str_eof:		LispBarf("Premature EOF in middle of string.");
		goto reterr;
	    }
	    if (ch == ESC_CHAR) {
		ch = f.Getc();
		if (ch == EOF)
		    goto str_eof;
		switch (ch) {
		    case 'n':
			ch = '\n';
			break;
		    case 't':
			ch = '\b';
			break;
		    case 'r':
			ch = '\r';
			break;
		    default:
			ch = ch;
		}
	    }
#if _UNICODE
	    if (ucol)
		TCB += ch;
	    else
#endif		
		CB += ch;
	}
#if _UNICODE
	if (ucol) {
            v1.u.S = intern_string(std::move(TCB)); // define this when needed
	}
	else {
#endif
            v1.u.s = intern_string(std::move(CB));
#if _UNICODE
	    _Lisp_Uncodize_Sym_Hook(&v1);
	}
#endif
	v1.type = L_STRING;
	ch = ' ';
    }
    else if (ch == ')') {
	if (listing) {
	    if (oc > 0)
		CDR(Last_Cons) = NIL;
	    else
		return NIL;
	    return First_Cons;
	}
	else {
	    LispBarf ("Unexpected list ')'");
reterr:	    v1 = READ_ERROR_OBJ;
	    goto retv1;
	}
    }
    else if (ch == ']') {
	if (vectoring) {
	    int elts = Stack_count-stack_base;
	    v1.type = L_VECTOR;
	    v1.u.l = new Sexpr[elts+1];
	    v1.u.l->type = L_NUM;
	    v1.u.l->u.n = elts;
	    for (int i = 0; i < elts; i++)
		v1.u.l[i+1] = Stack[stack_base+i];
	    goto retv1;
	}
	else {
	     LispBarf ("Unexpected vector ']'");
	     goto reterr;
	}
    }
#if ! _BLISP
    else if (ch == '!') {
	notf = 1 - notf;
	ch = f.Getc();
	goto more_lf;
    }
#endif
    else if (ch == '.') {
	if (!isdigit(f.Peek())) {
	    if (!listing){
dce:		LispBarf ("Reader dot context error.");
		goto reterr;
	    }
	    if (oc == 0)
		goto dce;
	    CDR(Last_Cons) = read_sexp_i(f, 5);
	    ch = skip_whitespace (f, ' ');
	    if (ch != ')') {
		LispBarf ("Dot CDR not followed by close paren.");
		goto reterr;
	    }
	    return First_Cons;
	}
	num = 0;
col_flonum_got_num:
	ch = f.Getc();
	if (!isdigit(ch)) {
	    v1.type = L_NUM;
	    v1.u.n = num;
	}
	else {
	    double fl = num;
	    double divi = 1;
	    for (num = 0; isdigit (ch); ch = f.Getc()) {
		fl = fl * 10.0 + (ch - '0');
		divi *= 10.0;
	    }
	    v1 = Sexpr(fl/divi);
	    goto ungnumch;
	}
    }
    else if (ch == '\'') {
	v1 = Lisp_Cons (QUOTE, Lisp_Cons (read_sexp_i (f, 5), NIL));
	ch = ' ';
    }
    else if (ch == '`') {
	v1 = Lisp_Cons (symBACKQUOTE, Lisp_Cons (read_sexp_i (f, 0), NIL));
	ch = ' ';
    }
    else if (ch == ',') {
	if (f.Peek() == '@') {
	    f.Getc();
	    v1 = CONS (symCOMMAATSIGN, CONS (read_sexp_i (f, 5), NIL));
	}
	else 
	    v1 = CONS (symCOMMA, CONS (read_sexp_i (f, 5), NIL));
	ch = ' ';
    }
    else {
	if (ch == EOF) {
	    LispBarf("Premature EOF/missing )");
	    goto reterr;
	}
	{
#if _UNICODE
icd:
#endif
	    char bb [100];
            // Actually putting the character in here, as before, causes Mac not to be able to UTF-8 it and you get nil.b
	    sprintf (bb, "Incomprehensible data in Lisp;\nFile position = %ld\nHex char = (#x%x)",
		     f.Tell(),  ch);
	    LispBarf (bb);
	}
	goto reterr;
    }
    if (notf) {
	notf = 0;
	v1 = Lisp_Cons (NOT, Lisp_Cons (v1, NIL));
    }
    if (!listing && !vectoring)
	goto retv1;
    if (vectoring)
	Stack[Stack_count++] = v1;
    else {
	Sexpr nc = Lisp_Cons (v1, NIL);
	if (oc++ == 0)
	    First_Cons = nc;
	else
	    CDR(Last_Cons) = nc;
	Last_Cons = nc;
    }
#if _UNICODE
    if (!I256p(ch))
	goto icd;
#endif
    if (isspace (ch) || ch == ';')
	ch = skip_whitespace (f, ch);
    goto more_lf;

}

Sexpr CreateRational (int numerator, int denominator) {
    Sexpr v1;
    v1.type = L_RATIONAL;
    v1.u.rat = new LRational;
    v1.u.rat->Numerator = numerator;
    v1.u.rat->Denominator = denominator;
    return v1;				/* no reduction yet! */
}

Sexpr CreateAtom (const char * s) {
    Sexpr v;
    v.type = L_ATOM;
    v.u.a = s;
    return v;
}


//---dam: Added prototype for read_sexp
//        since Code Warrior complains if there is no prototype
//        The prototype is not in lisp.h due to header file disorganization,
//        i.e. FILE is not declared in the right place
Sexpr read_sexp (FILE * f);
static std::string show_sexp_0(Sexpr);

Sexpr read_sexp (FILE * f) {
    Stack_count = 0;
    LispFileInputSource F(f);
    Sexpr S = read_sexp_i (F, 0);
#if 0
    printf("%s\n", show_sexp_0(S));
#endif
    return S;
}

//  1-22-2021  -- Used for skipping so next object's file position can be ascertained.
char skip_lisp_file_whitespace (FILE* f) {
    LispFileInputSource F(f);
    char ch = skip_whitespace(F, ' ');
    ungetc(ch, f);
    return ch;
}

Sexpr read_sexp_from_string (LispTChar * s, int *leftp) {
    Stack_count = 0;
    LispStringInputSource F(s);
    Sexpr v = read_sexp_i (F, 0);
    if (leftp)
	*leftp = F.GetIndex();
    return v;
}


Sexpr read_sexp_from_char_string (char * s, int *leftp) {
    Stack_count = 0;
    LispNarrowStringInputSource F(s);
    Sexpr v = read_sexp_i (F, 0);
    if (leftp)
	*leftp = F.GetIndex();
    return v;
}

Sexpr read_sexp_LIS (LispInputSource &Lis) {
    Stack_count = 0;
    return read_sexp_i (Lis, 0);
}

static std::string show_sexp_0 (Sexpr s) {
    switch (s.type) {
	case L_NULL:
            if (s.u.v == nullptr)
                return "%L_NULL<0>";
            else
                return FormatString("%%L_NULL<%s>", s.u.s);
	case L_ATOM:
            return s.u.a;
	case L_NUM:
            return std::to_string(s.u.n);
	case L_RATIONAL:
            return std::to_string((long)s.u.rat->Numerator) + FRACTION_BARs +
                    std::to_string((long)s.u.rat->Denominator);
	case L_FLOAT:
            return std::to_string(*s.u.f);
	case L_STRING:
        {
            std::string b;
	    b += '"';
	    for (const char * q = s.u.s; *q; q++) {
		if (*q == '"' || *q == ESC_CHAR)
		    b += ESC_CHAR;
		b += *q;
	    }
	    b += '"';
            return b;
        }
	case L_CHAR:
	    return "#" + ESC_CHARs + s.u.c;
	case L_CONS:
        {
            std::string b;
            b += '(';
            while(true) {
		b += show_sexp_0 (CAR(s));
		if (NILP (CDR(s)))
		    break;
		if (!(CONSP (CDR(s)))) {
		    b += " . ";
		    b += show_sexp_0(CDR(s));
		    break;
		}
                s = CDR(s);
                b += ' ';
            }
   	    b += ')';
	    return b;
        }
	case L_VECTOR:
	{
            std::string b;
	    b += "[";
	    int lim = (int)s.u.l[0].u.n;
	    for (int i = 0; i < lim; i++) {
		b += show_sexp_0 (s.u.l[i+1]);
		if (i < lim-1)
		    b += ' ';
	    }
            b += ']';
	    return b;
	}
	case L_RLYSYM:
            return s.u.r->PRep();
        case L_LAST_TYPE:
            return "<LAST-TYPE>";
    }
}

std::string Sexpr::PRep() {
    return show_sexp_0(*this);
}

void show_sexp (Sexpr s) {
    MessageBoxA (0, show_sexp_0(s).c_str(), "Lisp Exp", MB_OK);
}

void dealloc_ncyclic_sexp (Sexpr s) {
    int i;
    Sexpr s2;
loop:
    switch (s.type) {
	case L_NULL:
	    return;
	case L_ATOM:
	    return;			/* leave obarray gravid */
	case L_NUM:
	    return;
	case L_STRING:
	    break;
	case L_RLYSYM:
	    break;
	case L_CHAR:
	    break;
	case L_CONS:
	    dealloc_ncyclic_sexp(CAR(s));
	    s2 = s;
	    s = CDR(s);
//	    delete s2.u.l;
	    goto loop;
	case L_LIST:
	{
	    int lim = (int)s.u.l[0].u.n;
	    for (i = 0; i < lim; i++)
		dealloc_ncyclic_sexp (s.u.l[i+1]);
//	    delete s.u.l;
	    return;
	}
	case L_RATIONAL:
	    delete s.u.rat;
	    break;
	case L_FLOAT:
	    delete s.u.f;
	    break;
        case L_LAST_TYPE:
            LispBarf("Dealloc last type!?");
            break;
    }
}

#if ! _BLISP
void dealloc_lisp_sys() {
    MacroCleanup();
    AtomMap.clear();
#if ! RELAYS
    ClearRelayMaps();
#endif
}
#endif

#if ! DONT_DEFINE_LISPBARF
void LispBarf(int no, const char * cstr) {
    std::vector<Sexpr>v;
    LispBarfVariadic(no, cstr, v);
}
void LispBarf(int no, const char * cstr, Sexpr s1) {
    std::vector<Sexpr>v({s1});
    LispBarfVariadic(no, cstr, v);
}
void LispBarf(int no, const char * cstr, Sexpr s1, Sexpr s2) {
    std::vector<Sexpr>v({s1, s2});
    LispBarfVariadic(no, cstr, v);
}


void LispBarfVariadic (int n, std::string cmsg, std::vector<Sexpr>& sexps) {
    std::string msg = cmsg;
    if (sexps.size())
        msg += ":\n";

    for (Sexpr s : sexps) {
        if (msg.size())
            msg += ' ';
        msg += s.PRep();
    }
    MessageBoxA (0, msg.c_str(), LispBarfString, MB_OK|MB_ICONSTOP);
}
#endif  //DONT_DEFINE_LISPBARF

Sexpr LGetProp (Sexpr l, Sexpr p) {
    for (; CONSP(l) && CONSP (CDR(l)) ; l = CDDR(l))
	if (p == CAR(l))
	    return CADR(l);
    return NIL;
}

#if ! TLEDIT
bool TestRunExprf(const char * fpathname) {
    FILE * f = fopen (fpathname, "r");
    if (f == NULL) {
        fprintf (stderr, "Can't open %s.\n", fpathname);
        return false;
    };
    
    for (;;) {
        Sexpr v = read_sexp (f);
        show_sexp (v);
        printf ("\n");
        if (v.type == L_CONS && CAR(v) == DEFRMACRO) {
            printf ("Defining a macro...\n");
            int r = defrmacro (v);
            printf ("Retval is %d.\n", r);
        }
        Sexpr v2 = MaybeExpandMacro (v);
        if (v2 != EOFOBJ) {
            printf ("Macro was expanded!:\n");
            show_sexp (v2);
            printf ("\n");
        }
        if (v.type == L_NULL)
            break;
    }
    fclose (f);
    return true;
}

#if TEST
int main (int argc, char ** argv) {
    if (argc < 2) {
	fprintf (stderr, "Arg missing.\n");
	exit (2);
    }
    if (TestRunExprf(argv[1])) {
        exit (0);
    } else {
        exit(1);
    }
}
#endif

#endif
