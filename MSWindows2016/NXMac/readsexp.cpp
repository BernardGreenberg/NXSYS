/* "dam" is the venerable Dave Moon, who hacked this pgm in Sept 1996
    to port my music player to the Macintosh.  -bsg 2 October 1996 */

//---dam: Added Macintosh conditionalization
#include "oscond.h"

#include <stdlib.h>

#ifdef _UNICODE
#include <tchar.h>
#endif

#ifdef _BLISP
#define GOOD_SYM_DELIMS "&+-_@$%*=:<>!?"
#else
#define GOOD_SYM_DELIMS "&+-_@$%*=:"
#endif

#ifdef TEST
#define DOS 1
#endif

#ifdef DOS
#define LispCrash() exit(1);
#else
//---dam: Added Macintosh conditionalization
#ifdef MAC_OS
//#include <Types.h>
void LispCrash() {exit(3);}
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
extern "C" char * strupr(char *);
extern "C" int stricmp(const char *, const char *);
//#include "xstring.h"
#else
#define stricmp _stricmp
#endif
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <stdarg.h>
#include <string>
#include <vector>


#ifdef ZETALISP
#define ESC_CHAR '/'
#define FRACTION_BAR '\\'
#else
#define ESC_CHAR '\\'
#define FRACTION_BAR '/'
#endif

#ifdef _BLISP
#include <lispmath.h>
#endif

void LispBarf (int n, const char * s, ...);
static const char * LispBarfString = "Lisp Substrate";


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
	LispBarf (0, "Non-ASCII character in read-from-string string.");
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


void MacroCleanup();

void SetLispBarfString (const char * s) {
    LispBarfString = s;
}

const int HASHCT = 401; /* changed from 137 28 January 1998 */


#ifndef _BLISP
static std::vector<char *> Obarray;
#endif
static char Goodsymchar[256], Gsci =0, Fsci = 0;
static Rlysym **RObarray = NULL;
static std::vector<char *> RelayTypeTable;
static char RprBuf[20];

Sexpr NIL, NOT, ARG, FORMS, EOFOBJ, DEFRMACRO, QUOTE, BACKQUOTE,
    symCOMMA, symCOMMAATSIGN, READ_ERROR_OBJ, symFUNCTION;

LispCB1 LispReadTimeEvalHook = NULL;
LispCB1 LispLoadTimeEvalHook = NULL;


char * RlysymPRep (Sexpr s) {
    sprintf (RprBuf, "%ld%s", s.u.r->n, RelayTypeTable[s.u.r->type]);
    return RprBuf;
}

int ListLen (Sexpr s) {
	//---dam: Code Warrior warns for a null statement as a for body, so
	//        I changed it to an empty block
    int l = 0;
    for (l = 0; CONSP(s); SPop(s), l++) {}
    return l;
}

Sexpr Lisp_Cons (Sexpr s1, Sexpr s2) {
    Sexpr nc;
    nc.u.l = new Sexpr[2];
    if (nc.u.l == NULL) {
	LispBarf(0, "Lisp Cons alloc fails.");
	LispCrash();
    }
    nc.type = L_CONS;
    CAR(nc) = s1;
    CDR(nc) = s2;
    return nc;
}

static int CInit = 0;

static void MaybeInitLispSys () {
    if (!CInit)
	InitLispSys();
}


#ifndef _BLISP
Sexpr intern (const LispTChar * s) {

    MaybeInitLispSys();
    for (size_t i = 0; i < Obarray.size(); i++) {
	if (!stricmp (s, Obarray[i])) {
	    Sexpr sx;
	    sx.type = L_ATOM;
	    sx.u.a = Obarray[i];
	    return sx;
	}
    }
    char * ns =_strupr (_strdup (s));
    Obarray.push_back(ns);
    Sexpr sx;
    sx.type = L_ATOM;
    sx.u.a = ns;
    return sx;
}
#endif

#ifndef _BLISP
int get_noncanonical_relay_type_index (const char * s) {

    for (size_t i = 0; i < RelayTypeTable.size(); i++)
	if (!strcmp (RelayTypeTable[i], s))
	    return (int)i;
    RelayTypeTable.push_back(_strdup(s));   //DON'T STRUPR for NONCANONICAL
    return (int)RelayTypeTable.size()-1;
}


int get_relay_type_index (const char * s) {
    for (size_t i = 0; i < RelayTypeTable.size(); i++)
	if (!_stricmp (RelayTypeTable[i], s))
	    return (int)i;
    RelayTypeTable.push_back(_strupr(_strdup(s)));
    return (int)RelayTypeTable.size()-1;
}

const char * redeemRlsymId (int type) {
    if (type < 0 || type >= (int)RelayTypeTable.size()) {
        LispBarf(1, "Bad type index in redeemRlsymId: %d", type);
        return "??";
    }
    return RelayTypeTable [type];
}

int get_relay_array_for_object_number (int num, Relay** array) {
    int count = 0;
    for (int hcx = 0; hcx < HASHCT; hcx++)
	for (Rlysym* rsp = RObarray[hcx]; rsp != NULL; rsp = rsp->next)
	    if (rsp->n == num) {
		if (rsp->rly){ // no macro temp objects!
		    if (array)
			array[count] = rsp->rly;
		    count++;
		}
	    }
    return count;
}


Sexpr intern_rlysym_nocreate (long n, const char* str) {
    Rlysym rs;
    rs.n = n;
    rs.type = get_relay_type_index (str);
    int hcx = (rs.type*37 + rs.n)%HASHCT;
    
    for (Rlysym* rsp = RObarray[hcx];rsp != NULL; rsp = rsp->next)
	if (rsp->type == rs.type && rsp->n == rs.n) {
	    Sexpr s;
	    s.type = L_RLYSYM;
	    s.u.r = rsp;
	    return s;
	}
    return NIL;
}

Relay* get_relay_nocreate (long n, const char * str) {
    Sexpr S = intern_rlysym_nocreate(n, str);
    if (S == NIL)
	return NULL;
    else return S.u.r->rly;
}

#ifdef NXSYSMac
void DebugBreak();
#endif

Sexpr intern_rlysym (long n, const char* str) {
    Rlysym rs;
    rs.n = n;
    rs.type = get_relay_type_index (str);
    int hcx = (rs.type*37 + rs.n)%HASHCT;
        
    Rlysym * rsp = NULL;
    for (rsp = RObarray[hcx];rsp != NULL; rsp = rsp->next)
	if (rsp->type == rs.type && rsp->n == rs.n)
	    break;
    if (rsp == NULL) {
	rsp = new Rlysym;
	rsp->type = rs.type;
	rsp->n = rs.n;
	rsp->rly = NULL;
	rsp->next = NULL;
	rsp->next = RObarray[hcx];
	RObarray[hcx] = rsp;
    }
    Sexpr s;
    s.type = L_RLYSYM;
    s.u.r = rsp;
    return s;
}
#endif




static int Stack_count;
static Sexpr Stack[200];

#ifdef _UNICODE
#define I256p(x) ((x & 0xFF00) == 0)
#else
#define I256p(x) (1)
#endif

static int skip_whitespace (LispInputSource&f, int ch) {
top:
    while (I256p (ch) && isspace (ch))
	ch = f.Getc();
    if (ch == ';') {
	do ch = f.Getc();
	while (!(ch == EOF || ch == '\n'));
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
#ifdef _UNICODE
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
#ifdef _BLISP
	if (Goodsymchar[ch] && ch != FRACTION_BAR)
	    goto colsym;
#endif
	if (ch == FRACTION_BAR) {
	    int numerator = (int)num;
	    ch = f.Getc();
	    for (num = 0; isdigit (ch); ch = f.Getc())
		num = num * 10 + ch - '0';
	    if (num == 0) {
		LispBarf(0, "Zero denominator in rational fraction.");
		return NIL;
	    }
#ifdef _BLISP
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
		LispBarf (0, buf);
	    }
	}
	v1.type = L_NUM;
	v1.u.n = num*sign;
#ifdef TRACE_READ
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
		LispBarf (1, "Ignoring Load Time Eval, subbing NIL - #.", s);
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
pdbareof:		LispBarf (0, "End of file in middle of #| ... |#");
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
	    LispBarf (0, buf);
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
		LispBarf(0, "Symbol buffer overflow (256 chars max)");
		return NIL;
	    }
	    SymBuf[cct++] = ch;
	}
	SymBuf[cct] = '\0';
	if (rmacing)
	    f.Ungetc(ch);
	if (rlyf) {
	    rlyf = 0;
#ifdef _BLISP
	    LispBarf(0, "Invalid symbol (numbers followed by letters");
	    return NIL;
#else
	    _strupr(SymBuf);
	    v1 = intern_rlysym (num, SymBuf);
#endif
	}
	else
#ifdef _BLISP
	    v1 = TinternUC (SymBuf);
#else
	    v1 = intern (SymBuf);
#endif
#ifdef TRACE_READ
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
#ifdef _UNICODE
        std::wstring TCB;
#endif
	for (ch = f.Getc();ch != '"'; ch = f.Getc()) {
	    if (ch == EOF) {
str_eof:		LispBarf(0, "Premature EOF in middle of string.");
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
#ifdef _UNICODE
	    if (ucol)
		TCB += ch;
	    else
#endif		
		CB += ch;
	}
#ifdef _UNICODE
	if (ucol) {
	    v1.u.S = wstrdup(TCB.c_str()); // define this when needed
	    if (v1.u.S == NULL) {
		v1.u.S = (unsigned short *)malloc(sizeof(unsigned short));
		*v1.u.S = 0x0000;
	    }
	}
	else {
#endif
	    v1.u.s = _strdup(CB.c_str());
	    if (v1.u.s == NULL)
		v1.u.s = _strdup("");
#ifdef _UNICODE
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
	    LispBarf (0, "Unexpected list ')'");
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
	     LispBarf (0, "Unexpected vector ']'");
	     goto reterr;
	}
    }
#ifndef _BLISP
    else if (ch == '!') {
	notf = 1 - notf;
	ch = f.Getc();
	goto more_lf;
    }
#endif
    else if (ch == '.') {
	if (!isdigit(f.Peek())) {
	    if (!listing){
dce:		LispBarf (0, "Reader dot context error.");
		goto reterr;
	    }
	    if (oc == 0)
		goto dce;
	    CDR(Last_Cons) = read_sexp_i(f, 5);
	    ch = skip_whitespace (f, ' ');
	    if (ch != ')') {
		LispBarf (0, "Dot CDR not followed by close paren.");
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
	    v1 = CreateFlonum (fl/divi);
	    goto ungnumch;
	}
    }
    else if (ch == '\'') {
	v1 = Lisp_Cons (QUOTE, Lisp_Cons (read_sexp_i (f, 5), NIL));
	ch = ' ';
    }
    else if (ch == '`') {
	v1 = Lisp_Cons (BACKQUOTE, Lisp_Cons (read_sexp_i (f, 0), NIL));
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
	    LispBarf(0, "Premature EOF/missing )");
	    goto reterr;
	}
	{
#ifdef _UNICODE
icd:
#endif
	    char bb [100];
            // Actually putting the character in here, as before, causes Mac not to be able to UTF-8 it and you get nil.b
	    sprintf (bb, "Incomprehensible data in Lisp;\nFile position = %ld\nHex char = (#x%x)",
		     f.Tell(),  ch);
	    LispBarf (0, bb);
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
#ifdef _UNICODE
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

Sexpr CreateFlonum (double fl) {
    Sexpr v;
    v.type = L_FLOAT;
    v.u.f = new double;
    *v.u.f = fl;
    return v;
}

Sexpr CreateFixnum (long l) {
    Sexpr v;
    v.type = L_NUM;
    v.u.n = l;
    return v;
}

//---dam: Changed this function to static and gave it a prototype
//        since Code Warrior complains if there is no prototype

static void init_good_sym_chars (void);

static void init_good_sym_chars () {
    for (int i = 0; i < 256; i++)
	Goodsymchar[i] = isalnum(i);
    for (const char * ch = GOOD_SYM_DELIMS ;*ch; ch++)
	Goodsymchar[*ch] = 1;
    Goodsymchar[FRACTION_BAR] = 1;
}

void LispCleanOutRelays() {
    MacroCleanup();
    for (int i = 0; i < HASHCT; i++) {
	Rlysym * nnext;
	for (Rlysym* rsp = RObarray[i];rsp != NULL; rsp = nnext) {
	    nnext = rsp->next;
	    delete rsp;
	}
	RObarray[i] = NULL;
    }
}

void InitLispSys() {
    CInit = 1;
    if (!Gsci) {
	init_good_sym_chars();
	Gsci = 1;
    }
    if (!Fsci) {
        RObarray = new Rlysym*[HASHCT];
	for (int i = 0; i < HASHCT; i++)
	    RObarray[i] = NULL;
	BACKQUOTE = intern ("SI:BACKQUOTE");
	symCOMMA = intern ("SI:COMMA");
	symCOMMAATSIGN = intern ("SI:COMMAATSIGN");
	QUOTE = intern ("QUOTE");

	NOT = intern ("NOT");
	NIL = intern ("NIL");
	symFUNCTION = intern("FUNCTION");
	EOFOBJ.type = L_NULL;
	EOFOBJ.u.const_s = "EOF";
	READ_ERROR_OBJ.type = L_NULL;
	READ_ERROR_OBJ.u.const_s = "READ_ERROR";
	FORMS = intern ("FORMS");
	DEFRMACRO = intern ("DEFRMACRO");
	ARG = intern ("ARG");
	Fsci = 1;
    }
}

//---dam: Added prototype for read_sexp
//        since Code Warrior complains if there is no prototype
//        The prototype is not in lisp.h due to header file disorganization,
//        i.e. FILE is not declared in the right place
Sexpr read_sexp (FILE * f);
static void show_sexp_1(Sexpr, std::string&);

Sexpr read_sexp (FILE * f) {
    MaybeInitLispSys();
    Stack_count = 0;
    LispFileInputSource F(f);
    Sexpr S = read_sexp_i (F, 0);
#if 0
    std::string b;
    show_sexp_1(S, b);
    printf("%s\n", b.c_str());

#endif
    return S;
}

Sexpr read_sexp_from_string (LispTChar * s, int *leftp) {
    MaybeInitLispSys();
    Stack_count = 0;
    LispStringInputSource F(s);
    Sexpr v = read_sexp_i (F, 0);
    if (leftp)
	*leftp = F.GetIndex();
    return v;
}


Sexpr read_sexp_from_char_string (char * s, int *leftp) {
    MaybeInitLispSys();
    Stack_count = 0;
    LispNarrowStringInputSource F(s);
    Sexpr v = read_sexp_i (F, 0);
    if (leftp)
	*leftp = F.GetIndex();
    return v;
}

Sexpr read_sexp_LIS (LispInputSource &Lis) {
    MaybeInitLispSys();
    Stack_count = 0;
    return read_sexp_i (Lis, 0);
}

static void show_sexp_1 (Sexpr s, std::string& b) {
    char bb[64]; // for small flona, etc.
    int i;
    const char * q;
    switch (s.type) {
	case L_NULL:
	    b += "*EOF*";
	    return;
	case L_ATOM:
            b += s.u.a;
	    return;
	case L_NUM:
	    sprintf (bb, "%ld", s.u.n);
            b += bb;
            return;
	case L_RATIONAL:
	    sprintf (bb, "%ld%c%ld",
		     (long)s.u.rat->Numerator, FRACTION_BAR, (long)s.u.rat->Denominator);
	    b += bb;
	    return;
	case L_FLOAT:
	    sprintf (bb, "%f", *s.u.f);
            b += bb;
	    return;
	case L_STRING:
	    b += '"';
	    for (q = s.u.s; *q; q++) {
		if (*q == '"' || *q == ESC_CHAR)
		    b += ESC_CHAR;
		b += *q;
	    }
	    b += '"';
	    break;
	case L_CHAR:
	    sprintf (bb, "#%c%c", ESC_CHAR, s.u.c);
	    b += bb;
	    break;
	case L_CONS:
            b += '(';
            while(1) {
		show_sexp_1 (CAR(s), b);
		if (NILP (CDR(s)))
		    break;
		if (!(CONSP (CDR(s)))) {
		    b += " . ";
		    show_sexp_1(CDR(s), b);
		    break;
		}
                s = CDR(s);
                b += ' ';
            }
   	    b += ')';
	    return;
	case L_VECTOR:
	{
	    b += "[";
	    int lim = (int)s.u.l[0].u.n;
	    for (i = 0; i < lim; i++) {
		show_sexp_1 (s.u.l[i+1], b);
		if (i < lim-1)
		    b += ' ';
	    }
            b += ']';
	    return;
	}
	case L_RLYSYM:
	    b += RlysymPRep (s);
	    return;
        case L_LAST_TYPE:
            sprintf(bb, "<LAST-TYPE>");
            b += bb;
            break;
    }
}

char * SexpPRep (Sexpr s, char * b) {
    std::string realbuf;
    show_sexp_1 (s, realbuf);
    strcpy(b, realbuf.c_str()); //   this is only used in error blowouts -- maybe improve.
    for (size_t l = strlen(b); l > 0 && b[l-1] == ' '; l--)
       	b[l-1] = '\0';		/* drives (oo) up a tree, so to speak*/
    return b;

}


void show_sexp (Sexpr s) {
    std::string buffer;
    show_sexp_1 (s, buffer);
#ifdef WINDOWS
#ifdef LISP_ERRORS_TO_CIO
    printf ("%s", buffer);
#else
    MessageBoxA (0, buffer.c_str(), "Lisp Exp", MB_OK);
#endif
#else
    printf ("%s", buffer.c_str());
#endif
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
//	    free (s.u.s);
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
            LispBarf(1, "Dealloc last type!?");
            break;
    }
}


#ifndef _BLISP
void dealloc_lisp_sys() {
    if (!Fsci)
	return;
    MacroCleanup();

    for (size_t i = 0; i < Obarray.size(); i++)
	free (Obarray[i]);
    Obarray.clear();
    Rlysym *next;
    for (size_t i = 0; i < HASHCT; i++) {
	for (Rlysym* rsp = RObarray[i];rsp != NULL; rsp = next) {
	    next = rsp->next;
	    delete rsp;
	}
	RObarray[i] = NULL;
    }
    Fsci = 0;
}


void map_relay_syms (RlySymFunarg fa) {
    Rlysym *next;
    int rct = 0;
    int loopct = 0;
    for (int i = 0; i < HASHCT; i++) {
        loopct = 0;
	for (Rlysym* rsp = RObarray[i];rsp != NULL; rsp = next, loopct++, rct++) {
	    next = rsp->next;
	    (fa) (rsp);
	}
    }
}

void map_relay_syms_for_validate (void (*fcn)(const Rlysym*, int)) {
    Rlysym *next;
    int rct = 0;
    int loopct =0;
    for (int i = 0; i < HASHCT; i++) {
        loopct = 0;
	for (Rlysym* rsp = RObarray[i];rsp != NULL; rsp = next, loopct++, rct++) {
	    next = rsp->next;
	    (fcn) (rsp, i);
	}
    }
}

Sexpr RlysymFromStringNocreate (const char * s) {
    char buf[50];
    long lval;
    const char *p = s;
    while (isspace (*p)) p++;
    const char* pp = p;
    while (isdigit (*pp)) pp++;
    strncpy (buf, p, pp-p)[pp-p] = '\0';
    sscanf (buf, "%ld", &lval);
    return intern_rlysym_nocreate (lval, pp);
}

static int relaysym_ctr; //yuuuuc
bool relay_has_exp(Relay*); // yuuuuuk
static void count_relay_sym(void *vrp) {
#ifndef TLEDIT
    Rlysym * rsp = (Rlysym*)vrp;
    Relay * r = rsp->rly;
    if (r && relay_has_exp(r))
#endif
        relaysym_ctr++;
}

int CountRelaySyms() {
    relaysym_ctr = 0;
    map_relay_syms(count_relay_sym);
    return relaysym_ctr;
}


#endif

#ifndef DONT_DEFINE_LISPBARF
void LispBarf (int n, const char * s, ...) {
    std::string msg;
    msg = s;
    va_list (ap);
    va_start (ap, s);
    while (n-- > 0) {
        msg += ' ';
	show_sexp_1 (va_arg (ap, Sexpr), msg);
    }
    const char * buf = msg.c_str();
#ifdef WINDOWS
#ifdef LISP_ERRORS_TO_CIO
    fprintf (stderr, "%s: %s\n", LispBarfString, buf);
#else //not LISP_ERRORS_TO_CIO
    MessageBoxA (0, buf, LispBarfString, MB_OK|MB_ICONSTOP);
#endif
#else //macintosh
    MessageBox (0, buf, LispBarfString, 0);
#endif
}
#endif

Sexpr LGetProp (Sexpr l, Sexpr p) {
    for (; CONSP(l) && CONSP (CDR(l)) ; l = CDDR(l))
	if (p == CAR(l))
	    return CADR(l);
    return NIL;
}

#ifndef TLEDIT
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

#ifdef TEST
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
