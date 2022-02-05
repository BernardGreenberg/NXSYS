#include "windows.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "nxgo.h"
#include "commands.h"
#include "lisp.h"
#include "objid.h"
#include "compat32.h"
#include "trainapi.h"
#include "trainaut.h"
#include "lispmath.h"
#include "brushpen.h"
#include "demoapi.h"
#include "nxsysapp.h"
#include "incexppt.h"
#include "usermsg.h"
#include "ssdlg.h"

Sexpr read_sexp (FILE*);
#ifdef NXOLE
void ScriptPause(int haltsw);
#endif

#ifdef NXSYSMac
#include "AppDelegateGlobals.h"
void ShowBigYellowX(int x, int y);
void UnShowBigYellowX();
#endif

#ifndef NXSYSMac
#define stricmp _stricmp
#endif

#include "timers.h"

int FindHitSignal (long id, int&x, int&y, int but);
int RelayShowString (const char *);

static char T;
static HWND DemoWindow = NULL;

#define CLICK_TIME_DEFAULT_MS 1200
static long ClickTimeMs = CLICK_TIME_DEFAULT_MS;
static int DemoInProgress = 0, DemoPaused = 0, XWait = 0, BlurbUp = 0,
    DontDrawXes = 0;
static FILE* DemoFile = NULL;
static char DemoPath[MAXPATH];


extern unsigned smeasure (HDC dc, char * str);


RECT RR;

#ifdef NXV2

GraphicObject * FindDemoHitTurnout (long id);
GraphicObject * FindDemoHitCircuit (long id);

static  GraphicObject *  FindDemoObjectByID (long id, int key) {
    switch (key) {			/* as it were!!!!! */
	case ID_SIGNAL:
	case ID_SWITCHKEY:
	case ID_EXITLIGHT:
	case ID_PLATFORM:
	case ID_TRAFFICLEVER:
	    return FindHitObject (id, key);
	case ID_TURNOUT:
	    return FindDemoHitTurnout (id);
	case ID_TRACKSEC:
	    return FindDemoHitCircuit (id);
	default:
	    return NULL;
    }
}
#endif


void DemoTrain (Sexpr);

static int symcmp (Sexpr& s, const char * str) {
    if (s.type != L_ATOM)
	return 0;
    return (!strcmp (s.u.a, str));
}


#ifndef NXSYSMac
static int HalfX = 15;

static void DrawX (HDC dc) {
    MoveTo (dc, RR.left, RR.top);
    LineTo (dc, RR.right, RR.bottom);
    MoveTo (dc, RR.right, RR.top);
    LineTo (dc, RR.left, RR.bottom);
}

static void UnDrawX (HWND w, RECT& r) {
    SC_cord d = 2*GU2;
    RECT q = r;
    q.left -=  d;
    q.right += d;
    q.top -= d;
    q.bottom += d;
    InvalidateRect (w, &q, 1);
}

static void MakeX (SC_cord x, SC_cord y) {
    HDC dc = GetDC (G_mainwindow);
    RR.left = x-HalfX;
    RR.right = x + HalfX;
    RR.top = y - HalfX;
    RR.bottom = y + HalfX;
    SelectObject (dc, YellowXPen);
    DrawX (dc);
    ReleaseDC (G_mainwindow, dc);
}
#endif

static void EndDemo() {
    if (DemoFile != NULL)
	fclose (DemoFile);
    KillOneTimer (&T);
    XWait = 0;
    DemoFile = NULL;
    DemoInProgress = 0;
    HideDemoWindow();
}

void HideDemoWindow () {
    BlurbUp = 0;
#ifdef NXSYSMac
    MacDemoHide();
#else
    ShowWindow (DemoWindow, SW_HIDE);
    InvalidateRect (G_mainwindow, NULL, 0);
#endif
}


static void DemoImpulse (void*);
static BOOL DecodeOptions(Sexpr);
static void DemoTimer (long intvl) {
    NXTimer (&T, DemoImpulse, intvl);
}


static void DemoErrmsg (const char * text, ...) {
    char buf [400];
    va_list ap;
    va_start (ap, text);
    vsprintf (buf, text, ap);
    va_end (ap);

    MessageBox (G_mainwindow, buf,
		PRODUCT_NAME " demo script", MB_OK|MB_ICONEXCLAMATION);
}


void DemoSay (const char * msg) {
#ifdef NXSYSMac
    MacDemoSay(msg);

#else
    if (!!strcmp (msg, ""))
	ShowWindow (DemoWindow, SW_SHOW);
    NBDSetWindowText (DemoWindow, msg);
#endif
}


static void DemoImpulse (void*) {
    if (BlurbUp & !DemoInProgress)
	EndDemo();
    if (!DemoInProgress)
	return;
    if (DemoPaused) {
	if (DemoPaused == 1) {
	    DemoSay("SPACE to resume, ESC to end demo.  Demo paused.");
	    DemoPaused = 2;
	}
	return;
    }
    if (XWait) {
	if (XWait == 1) {

#ifdef NXSYSMac
            UnShowBigYellowX();
#else
            UnDrawX (G_mainwindow, RR);
#endif
	    XWait = 2;
	    DemoTimer(100);
	    return;
	}
	else XWait = 0;
    }

    Sexpr s, ss;
    ss.type = s.type = L_NULL;
    for (;;) {
	int key;
	int mousecmd = WM_LBUTTONDOWN;
	s = ss = read_sexp  (DemoFile);
top:
	if (s.type == L_NULL) {
ed:	    EndDemo();
	    break;
	}
	if (s.type != L_CONS) {
non_list:    DemoErrmsg ("Non-list in demo script.");
	    goto ed;
	}
	if (CDR(s).type != L_CONS)
	    goto non_list;

	Sexpr fn = CAR(s);
	if (symcmp (fn, "SAY")) {
	    if (CADR(s).type != L_NUM) {
		DemoErrmsg ("Missing wait # in SAY in demo script.");
		goto ed;
	    }
	    if (CDR(CDR(s)).type !=L_CONS) {
ns:		DemoErrmsg  ("Missing stringin SAY in demo script.");
		goto ed;
	    }
	    if (CADR(CDR(s)).type !=L_STRING)
		goto ns;
	    DemoSay (CADR(CDR(s)).u.s);
	    if (CADR(s).u.n > 0) {
		DemoTimer(CADR(s).u.n);
		break;
	    }
	}
	else if (symcmp (fn, "WAIT")) {
	    if (CADR(s).type != L_NUM) {
		DemoErrmsg ("Missing wait ms count in WAIT in demo script.");
		goto ed;
	    }
	    DemoTimer (CADR(s).u.n);
	    break;
	}
	else if (symcmp (fn, "LOAD")) {
            std::string path;
            include_expand_path(DemoPath, CADR(s).u.s, path);
	    if(!GetLayout (path.c_str(), TRUE)) {
		EndDemo();
		break;
	    }
	}
	else if (symcmp (fn, "SIGNAL")) {
	    key = ID_SIGNAL;
gro:	    if (CDR(s).type != L_CONS) {
		DemoErrmsg ("Mouse hit form too short in demo script.");
		goto ed;
	    }
	    int x, y;
	    if (CADR(s).type != L_NUM) {
		DemoErrmsg ("No item number in mouse hit demo form.");
		goto ed;

	    }
	    if (CDR(CDR(s)).type == L_CONS){ /* Allow "no comment" */
		if (CADR(CDR(s)).type != L_STRING) {
		    DemoErrmsg ("No string in mouse hit demo form.");
		    goto ed;
		}
		DemoSay (CADR(CDR(s)).u.s);
	    }
	    else DemoSay("");
#ifdef NXV2
	    GraphicObject * g = FindDemoObjectByID (CADR(s).u.n, key);
#else
	    GraphicObject * g = FindHitObject (CADR(s).u.n, key);
#endif
	    if (g != NULL) {
		g->FindHitGo (x, y, mousecmd);
		if (!DontDrawXes) {
#ifdef NXSYSMac
                    ShowBigYellowX(x,y);
#else
		    MakeX (x, y);
#endif
                    XWait = 1;
		    DemoTimer (ClickTimeMs);
		    break;
		}
	    }
	}
	else if (symcmp (fn, "TRACK")) {
	    key = ID_TRACKSEC;
	    goto gro;
	}
	else if (symcmp (fn, "EXITLIGHT")) {
	    key = ID_EXITLIGHT;
	    goto gro;
	}
	else if (symcmp (fn, "MOUSERIGHT")) {
	    mousecmd = WM_RBUTTONDOWN;
	    s = CDR(s);
	    goto top;
	}
	else if (symcmp (fn, "MOUSELEFTSHIFT") || symcmp (fn, "FLEET")) {
	    mousecmd = WM_NXGO_LBUTTONSHIFT;
	    s = CDR(s);
	    goto top;
	}
	else if (symcmp (fn, "SWITCH")) {
	    key = ID_TURNOUT;
	    goto gro;
	}
#ifndef NOTRAINS
	else if (symcmp (fn, "TRAIN"))
	    DemoTrain (CDR (s));	/* continue, no time */
#endif
	else if (symcmp (fn, "CIRCUIT"))
	    for (Sexpr q = CDR (s); q != NIL; q= CDR(q)) {
		Sexpr e = CAR(q);
		if (e.type == L_RLYSYM) {
		    if (e.u.r->rly == NULL) {
			DemoErrmsg ("No Relay in relay sym in CIRCUIT %s",
				    RlysymPRep (e));
			goto cct;
		    }
		    RelayShowString (RlysymPRep (e));
		}
	    }
	else if (symcmp (fn, "CHECK")) {
	    for (Sexpr q = CDR (s); q != NIL; q= CDR(q)) {
		Sexpr e = CAR(q);
		int nval = 1;
ev:		if (e.type == L_RLYSYM) {
		    if (e.u.r->rly == NULL) {
			DemoErrmsg ("No Relay in relay sym in CHECK %s",
				    RlysymPRep (e));
			goto cct;
		    }
		    if (*((unsigned char *) (e.u.r->rly)) != nval) {
			DemoErrmsg ("Relay %s state is wrong, s/b %d",
				    RlysymPRep (e), nval);
		    }
		}
		else if (e.type == L_CONS && CADR(e).type == L_RLYSYM) {
		    e = CADR(e);
		    nval ^= 1;
		    goto ev;
		}
		else
		    DemoErrmsg ("Bad expression in CHECK.");
	    }
	}
	else if (symcmp (fn, "CLICKTIME")) {
	    ClickTimeMs = CADR(s).u.n;
	}
	else if (symcmp (fn, "OPTIONS"))
	    DecodeOptions(CDR(s));
	else if (symcmp (fn, "VERSION")) {
	    if (s.type != L_CONS || CADR(s).type != L_NUM || CADR(s).u.n != 1) {
		DemoErrmsg ("Demo script VERSION not 1.");
		dealloc_ncyclic_sexp (ss);
		EndDemo();
		return;
	    }
	}
	else {
            DemoErrmsg ("Unknown demo form: %s\r\nHalting demo.", fn);

            dealloc_ncyclic_sexp (ss);
            EndDemo();
            return;
        }
cct:
	dealloc_ncyclic_sexp (ss);
	ss.type = L_NULL;
    }
    if (ss.type != L_NULL)
	dealloc_ncyclic_sexp (ss);
}

BOOL DecodeOptions (Sexpr S) {
    for (; S.type == L_CONS; SPop (S)) {
	Sexpr O = (CAR(S));
	if (O.type != L_ATOM) {
	    DemoErrmsg ("Non-atomic-symbol option in OPTIONS form.");
	    return FALSE;
	}
	if (!stricmp (O.u.s, "NOXES"))
	    DontDrawXes = 1;
	else if (!stricmp (O.u.s, "NOSTOPS"))
	    ImplementShowStopPolicy (SHOW_STOPS_NEVER);
	else if (!stricmp (O.u.s, "SHOWSTOPS"))
	    ImplementShowStopPolicy (SHOW_STOPS_ALWAYS);
	else if (!stricmp (O.u.s, "SHOWSTOPSRED"))
	    ImplementShowStopPolicy (SHOW_STOPS_RED);
	else if (!stricmp (O.u.s, "MAXIMIZE")) {
	    ShowWindow (G_mainwindow, SW_SHOWMAXIMIZED);
	    UpdateWindow (G_mainwindow);
	}
	else {
	    DemoErrmsg ("Unknown Demo option: %s", O.u.s);
	    return FALSE;
	}
    }
    return TRUE;
}


#ifndef NOTRAINS
 void DemoTrain (Sexpr S) {

    if (S.type != L_CONS || CAR(S).type != L_NUM) {
	DemoErrmsg ("Missing numeric train number in TRAIN form.");
	return;
    }
    int train_no = (int) (CAR(S).u.n);
    SPop(S);

    if (S.type != L_CONS || CAR(S).type != L_ATOM) {
	DemoErrmsg ("Missing train command symbol in TRAIN %d form.", train_no);
	return;
    }
    char * cmd = CAR(S).u.s;
    SPop(S);

    if (!stricmp (cmd, "SPEED")) {
	if (S.type != L_CONS || !NUMBERP(CAR(S))) {
	    DemoErrmsg ("Missing numeric train speed in TRAIN %d SPEED",
			train_no);
	    return;
	}
	if (!TrainAutoSetSpeed (train_no, *LCoerceToFloat(CAR(S)).u.f))
	    DemoErrmsg ("Could not set speed for train# %d", train_no);
    }
    else if (!stricmp (cmd, "CREATE")) {
	if (S.type != L_CONS || CAR(S).type != L_NUM) {
	    DemoErrmsg ("Missing track # in TRAIN %d CREATE", train_no);
	    return;
	}
	long start_place_id = CAR(S).u.n;
	long options = 0;
	SPop(S);

	for (; S.type == L_CONS; SPop(S)) {
	    if (CAR(S).type != L_ATOM) {
		DemoErrmsg ("Non-atomic create option in train %d CREATE", train_no);
		return;
	    }
	    cmd = CAR(S).u.s;
	    if (!stricmp (cmd, "HALTED"))
		options |= TRAIN_CTL_HALTED;
	    else if (!stricmp (cmd, "HIDEDIALOG"))
		options |= TRAIN_CTL_HIDEDLG;
	    else if (!stricmp (cmd, "MINIMIZEDIALOG"))
		options |= TRAIN_CTL_MINDLG;
	    else if (!stricmp (cmd, "FREEWILL"))
		options |= TRAIN_CTL_FREEWILL;
	    else {
		DemoErrmsg ("Train %d CREATE: unknown create option: %s", cmd);
		return;
	    }
	}
	    
	if (!TrainAutoCreate (train_no, start_place_id, options))
	    DemoErrmsg ("Could not create train# %d", train_no);
    }
    else {
	int icmd;
	if (!TrainAutoLookupCommand (cmd, icmd))
	    DemoErrmsg ("Unknown TRAIN command: %s", cmd);
	else if (!TrainAutoCmd (train_no, icmd))
	    DemoErrmsg ("Could not TRAIN %d %s", train_no, cmd);
    }
}
#endif
    

/* this is an external API  -- see demoapi.h*/
void DemoPause (int haltsw) {
#ifdef NXOLE
    ScriptPause(haltsw);
#endif
    if (DemoInProgress) {
	if (haltsw)
	    EndDemo();
	else
	    if (DemoPaused) {
		DemoPaused = 0;
		DemoImpulse(NULL);
	    }
	    else {
		DemoPaused = 1;
		DemoSay("Demo pausing...control the interlocking yourself now...");
		DemoTimer(1000);
	    }
    }
}

/* this is an external API */
void Demo (const char * fname) {
    if (DemoInProgress){
	DemoErrmsg ("Demo already in progress!");
	return;
    }

    strcpy (DemoPath, fname);		/* for expand path */
    DemoFile = fopen (fname, "r");
    if (DemoFile == NULL) {
	usermsgstop ("Can't open demo file %s: %s.", fname, strerror(NULL));
	return;
    }
    TrainMiscCtl (CmKillTrains);
    DemoInProgress = 1;
    DemoPaused = 0;
    XWait = 0;
    DontDrawXes =0;
    ClickTimeMs = CLICK_TIME_DEFAULT_MS;
    DemoSay ("");
    DemoImpulse (NULL);
    return;
}

/* this is an external API */
void DemoBlurb (const char * s) {
    DemoSay (s);
    ShowWindow (DemoWindow, SW_SHOW);
    DemoTimer (5000);
    BlurbUp = 1;
}

void ResizeDemoWindowParent (HWND hWnd) {
    if (!DemoWindow)
	return;
#ifndef NXSYSMac
    TEXTMETRIC tm;
    RECT r;
    SendMessage (hWnd, WM_SETFONT, (WPARAM) GetStockObject (SYSTEM_FONT), 0L);
    HDC dc = GetDC (hWnd);
    SelectObject (dc, GetStockObject (SYSTEM_FONT));
    GetTextMetrics (dc, &tm);
    ReleaseDC (hWnd, dc);
    unsigned font_size = tm.tmHeight + tm.tmExternalLeading;
    GetClientRect (hWnd, &r);
    int main_width = r.right-r.left;
    int main_height = r.bottom-r.top;
    MoveWindow (DemoWindow, 
		(int)(main_width*.1),    /* x */
		main_height - font_size, /* y */
		(int)(main_width*.8),    /* width */
		font_size,
		TRUE);
#endif
}


void CreateDemoWindow (HWND hWnd) {
#ifndef NXSYSMac
  DemoWindow = CreateWindow ("STATIC", PRODUCT_NAME " relay interlocking simulator",
			     SS_SIMPLE | WS_CHILD | WS_VISIBLE,
			     100, 100, 100, 100,
			     hWnd, NULL, app_instance, NULL);

  ResizeDemoWindowParent (hWnd);
  ShowWindow (DemoWindow, SW_HIDE);
#endif
}

#ifdef NXSYSMac
bool DemoWindowFilterKey(long keyCode) {
    if (!DemoInProgress) {
        return false;
    } else {
        switch (keyCode) {
            case 53: // esc
                DemoPause(1);
                return true;
            case 49: // space (apparently, no, there is no include file)
                DemoPause(0);
                return true;
            default:
                return false;
        }
        
    }
}
#endif
