#include "windows.h"
#include "lisp.h"
#include "text.h"
#include <stdarg.h>
#include <string.h>
#include "usermsg.h"
#include "ctype.h"

/* 14 January 1998 */
#ifdef NXSYSMac
#define symcmp(x,y) (!strcmp((x.u.s), (y)))
#else
#define symcmp(x,y) (!strcmp((x.u.s), (y)))
#endif

#ifdef TLEDIT
void usererr (const char *, ...);
#define BARF(x) {usererr x ;return 0;};
#else
#define BARF(x) {usermsgstop x ;return 0;};
#endif

#ifndef STRINGP
#define STRINGP(x) ((x).type == L_STRING)
#endif


/* (TEXT "Frob st." xcord ycord
                  HEIGHT	15
		  WIDTH		5
		  WEIGHT	200
		  FACE		"HelenOfTroy"
		  ITALIC	T/NIL)

		  */
		  
int ProcessTextForm (Sexpr s) {
    const char * string;
    long x, y;

    if (!CONSP(s) || !STRINGP(CAR(s)))
	BARF (("Text string missing or not a string in TEXT layout form."));
    string = CAR(s).u.s;
    SPop(s);
    if (!CONSP(s) || !NUMBERP(CAR(s)))
pcnn:	BARF (("Panel coordinates missing, or not numbers, in TEXT layout form."));
    x = CAR(s).u.n;
#ifndef NXV2
    x *= 100;
#endif
    SPop(s);
    if (!CONSP(s) || !NUMBERP(CAR(s)))
	goto pcnn;
    y = CAR(s).u.n;
    SPop(s);

    LOGFONT lf;
    memset (&lf, 0, sizeof(lf));
    COLORREF Color = 0;
    BOOL ColorGiven = FALSE;

    while (CONSP (s)) {
	if (!CONSP (CDR(s)))
	    BARF (("Value missing (odd # of items) in Keyword/Value list in TEXT layout form."));
	Sexpr Keyword = CAR(s); SPop(s);
	Sexpr Value = CAR(s); SPop(s);
	if (!ATOMP(Keyword))
	    BARF (("Keyword in Keyword/Value list in TEXT not a symbol."));
	if (symcmp(Keyword, "HEIGHT")) {
	    if (!NUMBERP(Value))
		BARF (("Font height in TEXT form not a number."));

            int height = (int)Value.u.n;
            if (height > 0 && height < 100)
                lf.lfHeight = (int)Value.u.n;
#ifdef NXSYSMac
          //  lf.lfHeight = (int)(lf.lfHeight * .5);  // not understood
#endif
	}
	else if (symcmp (Keyword, "WEIGHT")) {
	    if (ATOMP(Value) && symcmp (Value, "BOLD"))
		lf.lfWeight = FW_BOLD;
	    else if (ATOMP(Value) && symcmp (Value, "NORMAL"))
		    lf.lfWeight = FW_NORMAL;
	    else if (NUMBERP(Value))
		lf.lfWeight =(int)Value.u.n;
	    else
		BARF (("Font weight in TEXT form not a number, BOLD, "
		       "or NORMAL."));
	}
	else if (symcmp (Keyword, "WIDTH")) {
	    if (!NUMBERP(Value))
		BARF (("Font width in TEXT form not a number."));
            int width = (int)Value.u.n;
            if (width > 0 && width < 100)
                lf.lfWidth = (int)Value.u.n;
	}
	else if (symcmp (Keyword, "FACE")) {
	    if (!STRINGP(Value))
		BARF (("Face name in TEXT form not a string."));
            if (strlen(Value.u.s) > 10
                || ! isalpha(Value.u.s[0]) || ! isalpha(Value.u.s[1])) {
                //bad stuff in file
            } else {
                strcpy (lf.lfFaceName, Value.u.s);
            }
	}
	else if (symcmp (Keyword, "ITALIC")) {
	    if (Value == NIL)
		lf.lfItalic = FALSE;
	    else if (symcmp (Value, "T"))
		lf.lfItalic = TRUE;
	    else
		BARF (("Font width in TEXT form not T or NIL."));
	}
	else if (symcmp (Keyword, "COLOR")) {
	    if (!CONSP(Value) || ListLen(Value)!=3)
badcol:		BARF (("Font color in TEXT form not list of 3 numbers (RGB)."));
	    if (!NUMBERP(CAR(Value))) goto badcol;
	    long r = CAR(Value).u.n; SPop(Value);
	    if (!NUMBERP(CAR(Value))) goto badcol;
	    long g = CAR(Value).u.n; SPop(Value);
	    if (!NUMBERP(CAR(Value))) goto badcol;
	    long b = CAR(Value).u.n;
	    Color = RGB(r, g, b);
	    ColorGiven = TRUE;
	}
	else BARF(("Unknown Font keyword in TEXT form: %s", Keyword.u.s));
    }

    LayoutTextString (string, &lf, x, y, Color, ColorGiven);
    return 1;
}
