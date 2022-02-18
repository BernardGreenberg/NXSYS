#include "lisp.h"

//---dam: add include of prototypes
#include "lispmath.h"

int LpZerop (Sexpr x) {
    switch (x.type) {
	case Lisp::FLOAT:
	    return *x.u.f == 0.0;
	case Lisp::NUM:
	    return (x.u.n == 0);
	case Lisp::RATIONAL:
	    return (x.u.rat->Numerator == 0);
	default:
	    return 0;
    }
}

Sexpr LCoerceToFloat (Sexpr x) {
    switch (x.type) {
	case Lisp::FLOAT:
	    return x;
	case Lisp::NUM:
	    return Sexpr ((double)x.u.n);
	case Lisp::RATIONAL:
	    return Sexpr
		    ((double)x.u.rat->Numerator/(double)x.u.rat->Denominator);
	default:
	    return x;
    }
}

static double CoerceToFloatVal (Sexpr x) {
    switch (x.type) {
	case Lisp::FLOAT:
	    return *x.u.f;
	case Lisp::NUM:
	    return (double)(x.u.n);
	case Lisp::RATIONAL:
	    return ((double)x.u.rat->Numerator/(double)x.u.rat->Denominator);
	default:
	    return 0.0;
    }
}

Sexpr LCoerceToFix   (Sexpr x) {
    switch (x.type) {
	case Lisp::FLOAT:
	    return Sexpr ((long)(*x.u.f));
	case Lisp::NUM:
	    return x;
	case Lisp::RATIONAL:
            return (Sexpr ((long)(x.u.rat->Numerator/x.u.rat->Denominator)));
	default:
	    return x;
    }
}


long GCD (long x, long y) {
    if (x < 0.0) x = -x;
    if (y < 0.0) y = -y;
    if (y > x) {
	long t = x;
	x = y;
	y = t;
    }
    while (y != 0) {
	long t = x % y;
	x = y;
	y = t;
    }
    return x;
}    

Sexpr CreateReducedRational (long num, long den) {

    if (num == 0)
	return Sexpr(0);
    int sign = 1;
    if (num < 0) {
	sign = -1;
	num = -num;
    }
    int gcd = (int)GCD (num, den);
    if (gcd == den)
	return Sexpr ((long)(sign*num/den));
    else return CreateRational ((int)(sign*num/gcd), int(den/gcd));
}

struct LMathRatArgs {
    long xn, xd, yn, yd;
    LMathRatArgs (Sexpr x, Sexpr y);
};

LMathRatArgs::LMathRatArgs (Sexpr x, Sexpr y) {
    if (x.type == Lisp::NUM) {
	xn = x.u.n;
	xd = 1;
    }
    else {
	xn = x.u.rat->Numerator;
	xd = x.u.rat->Denominator;
    }
    if (y.type == Lisp::NUM) {
	yn = y.u.n;
	yd = 1;
    }
    else {
	yn = y.u.rat->Numerator;
	yd = y.u.rat->Denominator;
    }
}
    

Sexpr LMultiply (Sexpr x, Sexpr y) {
    if (x.type == Lisp::FLOAT || y.type == Lisp::FLOAT)
	return Sexpr (CoerceToFloatVal(x)*CoerceToFloatVal(y));
    LMathRatArgs RA (x, y);
    return CreateReducedRational (RA.xn*RA.yn, RA.xd*RA.yd);
}

Sexpr LDivide   (Sexpr x, Sexpr y) {
    if (x.type == Lisp::FLOAT || y.type == Lisp::FLOAT)
	return Sexpr (CoerceToFloatVal(x)/CoerceToFloatVal(y));
    LMathRatArgs RA (x, y);
    return CreateReducedRational (RA.xn*RA.yd, RA.xd*RA.yn);
}


Sexpr LAdd (Sexpr x, Sexpr y) {
    if (x.type == Lisp::FLOAT || y.type == Lisp::FLOAT)
	return Sexpr (CoerceToFloatVal(x)+CoerceToFloatVal(y));
    LMathRatArgs RA (x, y);
    return CreateReducedRational (RA.xn*RA.yd + RA.xd*RA.yn, RA.xd*RA.yd);
}

Sexpr LSubtract (Sexpr x, Sexpr y) {
    if (x.type == Lisp::FLOAT || y.type == Lisp::FLOAT)
	return Sexpr (CoerceToFloatVal(x)-CoerceToFloatVal(y));
    LMathRatArgs RA (x, y);
    return CreateReducedRational (RA.xn*RA.yd - RA.xd*RA.yn, RA.xd*RA.yd);
}

int LCompare (Sexpr x, Sexpr y) {
    double xv, yv;
    if (x.type == Lisp::FLOAT && y.type == Lisp::FLOAT) {
	xv = *x.u.f;
	yv = *y.u.f;
cpfloat:
	if (xv > yv)
	    return 1;
	if (xv < yv)
	    return -1;
	else return 0;
    }
    if (x.type == Lisp::FLOAT || y.type == Lisp::FLOAT) {
	xv = CoerceToFloatVal(x);
	yv = CoerceToFloatVal(y);
	goto cpfloat;
    }
    LMathRatArgs RA (x, y);
    long xnum = RA.xn*RA.yd - RA.xd*RA.yn;
    if (xnum == 0L)
	return 0;
    if (xnum > 0L)
	return 1;
    else
	return -1;
}
