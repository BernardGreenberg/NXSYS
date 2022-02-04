#ifndef __BSG_LISP_MATH_H__
#define __BSG_LISP_MATH_H__

Sexpr LMultiply (Sexpr x, Sexpr y);
Sexpr LDivide   (Sexpr x, Sexpr y);
Sexpr LAdd      (Sexpr x, Sexpr y);
Sexpr LSubtract (Sexpr x, Sexpr y);
Sexpr LCoerceToFloat (Sexpr x);
Sexpr LCoerceToFix   (Sexpr x);
Sexpr CreateReducedRational (long num, long den);
int   LpZerop        (Sexpr x);
int   LCompare	     (Sexpr x, Sexpr y);

//---dam: Add missing prototype
long GCD (long x, long y);

#endif
