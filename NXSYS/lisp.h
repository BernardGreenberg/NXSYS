#ifndef _NX_LISP_SYS_H__
#define _NX_LISP_SYS_H__

/* hacked by BSG (and Dave Moon!) 9/96 for MusicSys - rationals, floats. */

enum class Lisp {tNULL = 0, VECTOR, STRING, ATOM, NUM, CHAR,
		    RLYSYM, tCONS, RATIONAL, FLOAT, LAST_TYPE};

#define LISP_TYPE_DESCRIPTIONS_NOBRACKETS \
 "NULL", "VECTOR", "STRING", "ATOMIC-SYMBOL", "FIXNUM", "CHARACTER", \
 "RELAY-SYMBOL", "CONS", "RATIONAL", "FLOAT", "LAST-TYPE"

#define LISP_TYPE_DESCRIPTIONS { LISP_TYPE_DESCRIPTIONS_NOBRACKETS}


#ifdef _UNICODE
typedef unsigned short  LispTChar;
#else
typedef char LispTChar;
#endif
#include <stdio.h>
#include <string>
#include <vector>
#include <cassert>

struct Rlysym;

struct LRational {
    int Numerator;
    int Denominator;
};

struct Sexpr {
    
    union {
	struct Sexpr * l;
        const char * s;    // consted 8/15/2019
	LispTChar *S;
	LispTChar *A;
	const char * a;    // consted 8/15/2019
	long   n;
	char   c;
	double * f;
	LRational * rat;
	Rlysym * r;
	const void * v;    // consted 11/23/2020
    } u;
    enum Lisp type;
    
    Sexpr() : Sexpr(Lisp::tNULL, nullptr) {}
    
    Sexpr(enum Lisp a_type, const void* v) {
        type = a_type;
        u.v = v;
    }

    explicit Sexpr(long longval) {
        type = Lisp::NUM;
        u.n = longval;
    }
    
    explicit Sexpr(int intval) {
        type = Lisp::NUM;
        u.n = intval;
    }
    
    explicit Sexpr(double dval) {
        type = Lisp::FLOAT;
        u.f = new double(dval);
    }
    
    explicit Sexpr(Rlysym* rsp) {
        type = Lisp::RLYSYM;
        u.r = rsp;
    }
    
    operator int () {
        return (int)u.n;
    }

    operator long () {
        return u.n;
    }

    int operator == (Sexpr& r) {return r.type == type && r.u.l == u.l;};
    int operator != (Sexpr& r) {return r.type != type || r.u.l != u.l;};

    std::string PRep() const;
};

//#ifndef _NX_SYS_RELAYS_H__
class Relay;
//#endif

typedef Sexpr *Sexp;

Sexpr intern (const char * s);
Sexpr Tintern (const LispTChar * s);
Sexpr TinternUC (const LispTChar * s);
Sexpr intern_rlysym (long n, const char * s);
Sexpr intern_rlysym_nocreate (long n, const char * s);

void show_sexp (Sexpr s);
void dealloc_ncyclic_sexp (Sexpr s);
int ListLen(Sexpr s);
void LispCleanOutRelays();
void dealloc_lisp_sys();
typedef void ((*RlySymFunarg)(Rlysym*rlysym,void* environment));
//MapperThunker.h does this much better, but this is left here for nostalgia.
// https://bannalia.blogspot.com/2016/07/passing-capturing-c-lambda-functions-as.html
#define MAP_RELAYSYMS_THUNK(thunk,lam)  auto thunk = [](Rlysym* rsp, void* closure_ptr) {(*static_cast <decltype(lam)*> (closure_ptr))(rsp);  };
typedef void (Rlysym::*RlysymMethodFunargType)();
void map_relay_syms (RlySymFunarg, void* opaque_ptr = nullptr);
void map_relay_syms_method(RlysymMethodFunargType) ;
Sexpr Lisp_Cons (Sexpr, Sexpr);
const char* redeemRlsymId (int type);

Sexpr RlysymFromStringNocreate (const char * s);
Sexpr CreateRational (int, int);

Sexpr LGetProp (Sexpr l, Sexpr p);

typedef Sexpr (*LispCB0)(void);
typedef Sexpr (*LispCB1)(Sexpr);
typedef Sexpr (*LispCB2)(Sexpr, Sexpr);
typedef Sexpr (*LispCB3)(Sexpr, Sexpr, Sexpr);

extern LispCB1 LispReadTimeEvalHook;
extern LispCB1 LispLoadTimeEvalHook;
extern void MacroCleanup();

int defrmacro (Sexpr arg);
int defrmacro_maybe_dup (Sexpr arg, int dup_ok);
Sexpr MaybeExpandMacro (Sexpr s);

struct Rlysym {
    Rlysym (long nomenc, short type_index, Relay* relay)
    : n(nomenc), type(type_index), rly(relay) {}
    std::string PRep();                 /* printed rep */

    long    n;				/* 4732 of 4732TP */
    short   type;			/* index to "TP" in relay-type tbl */
    Relay *rly;				/* pointer to actual relay */

    void DestroyRelayLogic();
    void DestroyRelay();
    
    bool operator < (const Rlysym& other) const {  // for UI's
        //must be LESS THAN to be true, not <=
        if (n == other.n)
            return strcmp(redeemRlsymId(type),  redeemRlsymId(other.type)) < 0 ;
        else
            return n < other.n;
    }
};

#define CAR(X) X.u.l[0]
#define CDR(X) X.u.l[1]
#define CAAR(X) (CAR(CAR(X)))
#define CADR(X) (CAR(CDR(X)))
#define CDDR(X) (CDR(CDR(X)))
#define CADDR(X) (CAR(CDR(CDR(X))))
#define CONSP(X) (X.type == Lisp::tCONS)
#define L_LIST L_VECTOR
extern Sexpr NIL, DEFRMACRO, EOFOBJ, READ_ERROR_OBJ, FORMS, ARG, QUOTE,
       symBACKQUOTE, symCOMMA, symCOMMAATSIGN, symFUNCTION;
#define NILP(x) (x.type == Lisp::ATOM && x.u.l == NIL.u.l)
#define ATOMP(x) (x.type == Lisp::ATOM)
#define NUMBERP(X) (X.type == Lisp::NUM || X.type == Lisp::RATIONAL || X.type == Lisp::FLOAT)
#define SPop(X) (X=CDR(X))
#define CONS Lisp_Cons

void LispBarfVariadic(int given_no, const std::string str, std::vector<Sexpr>& v);

//void LispBarf (int nsexps, const char * msg, ...);
void LispBarf(int no, std::string cstr);
void LispBarf(int no, std::string cstr, Sexpr s1);
void LispBarf(int no, std::string cstr, Sexpr s1, Sexpr s2);
inline void LispBarf(std::string cstr) {
    LispBarf(0, cstr);
}
inline void LispBarf(const char* cstr, Sexpr s1) {
    LispBarf(1, cstr, s1);
}
inline void LispBarf(const char* cstr, Sexpr s1, Sexpr s2) {
    LispBarf(2, cstr, s1, s2);
}

void SetLispBarfString (const char * s);
Sexpr read_sexp_from_string (const LispTChar * s, int*where_left);
Sexpr read_sexp_from_char_string (const char * s, int*where_left);
Sexpr read_sexp(FILE* f);
char skip_lisp_file_whitespace(FILE * f);

#ifdef __cplusplus

class LispInputSource  {
    public:
	int IsUnicode;
	LispInputSource() {IsUnicode = 0;};
	int virtual Getc() = 0;
	void virtual Ungetc(int c) = 0;
	long virtual Tell() = 0;
	int Peek();
};

Sexpr read_sexp_LIS (LispInputSource& Lis);

inline Sexpr SPopCar(Sexpr& L) {
    Sexpr car = CAR(L);
    assert (L.type == Lisp::tCONS);
    L = CDR(L);
    return car;
}

/* This solves the "static initialization order fiasco" created by
 former static Sexpr FOO = intern("FOO") in multiple files, that worked
 by accident. */
class self_snapping_Sexpref {   // 22 Nov 2020
    Sexpr m_value;
    const char * m_name;
public:
    self_snapping_Sexpref(const char * name) : m_name(name), m_value(Sexpr())  {}
    operator Sexpr& () {  //Sexpr == operator takes a ref.
        if (m_value.type == Lisp::tNULL) {
            m_value = intern(m_name);
        }
        return m_value;
    }
};

#define DEFLSYM2(sym,str) static self_snapping_Sexpref sym (#str)

#define DEFLSYM(x)  DEFLSYM2(x,x)
#define aDEFLSYM(x) DEFLSYM2(a##x, x)

extern "C" {
#endif
    void _Lisp_Uncodize_Sym_Hook(Sexpr * symptr);
#ifdef __cplusplus
}
#endif

#endif

