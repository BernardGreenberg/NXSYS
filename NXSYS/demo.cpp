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
#include "rlyapi.h"
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
#include <string>
#include <exception>
#include "STLExtensions.h"

/* Remodularized/rewritten/C++11 for no good reason 26 Sept 2019 */

Sexpr read_sexp (FILE*);
#ifdef NXOLE
void ScriptPause(int haltsw);
#endif

#ifdef NXSYSMac
#include "AppDelegateGlobals.h"
void ShowBigYellowX(int x, int y);
void UnShowBigYellowX();
#endif

#include "timers.h"

int FindHitSignal (long id, int&x, int&y, int but);
void RelayShowString (const char *);

static HWND DemoWindow = NULL;

#define CLICK_TIME_DEFAULT_MS 1200
static bool BlurbUp = false;

class DemoState {
public:
    long ClickTimeMs;
    bool DontDrawXes;
private:
    enum XWStates {NOT_XWAITING, XING, XENDING} XWait;
    enum DPStates {NOT_PAUSED, PAUSE1, PAUSE2} Paused;
    FILE* File;
    std::string RawPath;
public:
    DemoState(FILE*, const char* raw_fname);
    ~DemoState();
    
    void ProcessDemoPause(int haltsw);
    bool ProcessWaitStates();
    bool MakeBigX(GraphicObject* g, int mousecmd);
    Sexpr read() {
        return read_sexp(File);
    }
    std::string ExpandPath(const char* path) {
        return STLincexppath(RawPath, path);
    }
};

static std::unique_ptr<DemoState> State;

extern unsigned smeasure (HDC dc, char * str);

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

#ifndef NXSYSMac
RECT RR;

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

DemoState::~DemoState() {
    fclose (File);
    KillOneTimer (this);
    HideDemoWindow();
}

void HideDemoWindow () {
    BlurbUp = false;
#ifdef NXSYSMac
    MacDemoHide();
#else
    ShowWindow (DemoWindow, SW_HIDE);
    InvalidateRect (G_mainwindow, NULL, 0);
#endif
}


static void DemoImpulse (void*);
static void DecodeOptions(Sexpr);
static void DemoTimer(long intvl) {
    NXTimer (State.get(), DemoImpulse, intvl);
}

static void DemoErrmsg (const char * text, ...) {
    va_list ap;
    va_start (ap, text);
    std::string msg = FormatStringVA(text, ap);
    va_end (ap);
    MessageBox (G_mainwindow, msg.c_str(), PRODUCT_NAME " demo script", MB_OK | MB_ICONEXCLAMATION);
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

class DemoErr : public std::exception {
public:
    std::string message;
    DemoErr(const std::string beef) {
        message = beef;
    }
    DemoErr(const char * beef, int iarg) :
      DemoErr(FormatString(beef, iarg)) {}
};

class SHolder {  // This allows Sexpr to be deallocated unconditionally.
    Sexpr S;
public:
    SHolder (Sexpr s_) : S(s_) {}
    operator Sexpr() {
        return S;
    }
    ~SHolder () {
        dealloc_ncyclic_sexp(S);
    }
};

bool DemoState::MakeBigX(GraphicObject* g, int mousecmd) {
    int x, y;
    g->FindHitGo (x, y, mousecmd);
    if (!DontDrawXes) {
#ifdef NXSYSMac
        ShowBigYellowX(x,y);
#else
        MakeX (x, y);
#endif
        XWait = XING;
        DemoTimer(ClickTimeMs);
        return true;
    }
    return false;
}

static bool ProcessGOForm(int key, Sexpr s, int mousecmd) {
    if (CDR(s).type != Lisp::tCONS)
        throw DemoErr ("Mouse hit form too short in demo script.");
    if (CADR(s).type != Lisp::NUM)
        throw DemoErr ("No item number in mouse hit demo form.");
    if (CDR(CDR(s)).type == Lisp::tCONS){ /* Allow "no comment" */
        if (CADR(CDR(s)).type != Lisp::STRING)
            throw DemoErr ("No string in mouse hit demo form.");
        DemoSay (CADR(CDR(s)).u.s);
    }
    else DemoSay("");
#ifdef NXV2
    GraphicObject * g = FindDemoObjectByID (CADR(s).u.n, key);
#else
    GraphicObject * g = FindHitObject (CADR(s).u.n, key);
#endif
    if (g != NULL)
        if (State->MakeBigX(g, mousecmd))
            return true;
    return false;
}

static bool ProcessForm (Sexpr s, int mousecmd = WM_LBUTTONDOWN) {
    /* return true = break interpret loop for either end or wait */

    if (s.type == Lisp::tNULL) {
        State.reset();
        return true;
    }
    if (s.type != Lisp::tCONS || CDR(s).type != Lisp::tCONS)
        throw DemoErr ("Non-list in demo script.");
    if (CAR(s).type != Lisp::ATOM)
        throw DemoErr ("Non-atom at head of Demo command list.");
    
    std::string name = CAR(s).u.a;
    if (name == "OPTIONS")
        DecodeOptions(CDR(s));
    else if (name == "VERSION") {
        if (s.type != Lisp::tCONS || CADR(s).type != Lisp::NUM || CADR(s).u.n != 1)
            throw DemoErr ("Demo script VERSION not 1.");
    }
    else if (name == "SAY") {
        if (CADR(s).type != Lisp::NUM)
            throw DemoErr ("Missing wait # in SAY in demo script.");
        if (CDR(CDR(s)).type !=Lisp::tCONS || (CADR(CDR(s)).type !=Lisp::STRING))
            throw DemoErr ("Missing string in SAY in demo script.");

        DemoSay (CADR(CDR(s)).u.s);
        if (CADR(s).u.n > 0) {
            DemoTimer(CADR(s).u.n);
            return true;
        }
    }
    else if (name == "CLICKTIME")
        State->ClickTimeMs = CADR(s).u.n;
    else if (name == "WAIT") {
        if (CADR(s).type != Lisp::NUM)
            throw DemoErr ("Missing number in WAIT.");
        DemoTimer (CADR(s).u.n);
        return true;
    }
    else if (name == "LOAD") {
        std::string path = State->ExpandPath(CADR(s).u.s);
        if (!GetLayout (path.c_str(), TRUE)) {
            State.reset();  /* should have issued diagnostic already */
            return true;
        }
    }

    else if (name == "MOUSERIGHT")
        return ProcessForm(CDR(s), WM_RBUTTONDOWN);
    else if (name == "MOUSELEFTSHIFT" || name == "FLEET")
        return ProcessForm(CDR(s), WM_NXGO_LBUTTONSHIFT);

    else if (name == "SIGNAL")
        return ProcessGOForm(ID_SIGNAL, s, mousecmd);
    else if (name == "TRACK")
        return ProcessGOForm(ID_TRACKSEC, s, mousecmd);
    else if (name == "EXITLIGHT")
        return ProcessGOForm(ID_EXITLIGHT, s, mousecmd);
    else if (name == "SWITCH")
        return ProcessGOForm(ID_TURNOUT, s, mousecmd);

#ifndef NOTRAINS
    else if (name == "TRAIN")
        DemoTrain (CDR (s));    /* continue, no time */
#endif
    else if (name == "CIRCUIT")
        for (Sexpr q = CDR (s); q != NIL; q= CDR(q)) {
            Sexpr e = CAR(q);
            if (e.type == Lisp::RLYSYM) {
                std::string rname = e.u.r->PRep();
                if (e.u.r->rly == NULL)
                    throw DemoErr (std::string("No Relay in relay sym in CIRCUIT " + rname));
                RelayShowString (rname.c_str());
            }
        }
    else if (name == "CHECK") {
        auto valrelay = [](Rlysym* rs, int expected_value) {
            if (rs->rly == NULL)
                throw DemoErr (std::string("No Relay in relay sym in CHECK ") + rs->PRep());

            if (RelayState(rs->rly) != expected_value)
                DemoErrmsg ("Relay %s state is wrong, s/b %d",
                            rs->PRep().c_str(), expected_value);
        };
        for (Sexpr q = CDR (s); q != NIL; q = CDR(q)) {
            Sexpr e = CAR(q);
            if (e.type == Lisp::RLYSYM)
                valrelay(e.u.r, 1);
            else if (e.type == Lisp::tCONS && CADR(e).type == Lisp::RLYSYM)
                /* "I hope NOT!" */
                valrelay(CADR(e).u.r, 0);
            else
                throw DemoErr ("Bad expression in CHECK.");
        }
    }
    else {
        std::string complaint
          (FormatString("Unknown demo form: %s\r\nHalting demo.", name.c_str()));
        throw DemoErr (complaint);
    }
    return false;  /* continue interpreting forms */
}

bool DemoState::ProcessWaitStates() {
    switch(Paused) {
        case NOT_PAUSED:
            break;
        case PAUSE1:
            DemoSay("SPACE to resume, ESC to end demo.  Demo paused.");
            Paused = PAUSE2;
            return true;
        case PAUSE2:
            Paused = NOT_PAUSED;
            break;
    }

    switch (XWait) {
        case NOT_XWAITING:
            break;
        case XING:
#ifdef NXSYSMac
            UnShowBigYellowX();
#else
            UnDrawX (G_mainwindow, RR);
#endif
            XWait = XENDING;
            DemoTimer(100);
            return true;
        case XENDING:
            XWait = NOT_XWAITING;
            break;
    }
    return false;
}


static void DemoImpulse (void*) {

    if (!State) {      /* no demo going on, no state */
        if (BlurbUp) {
            BlurbUp = false;
            HideDemoWindow();
        }
	return;
    }

    if (State->ProcessWaitStates())
        return;

    try {
        for (bool exitf = false; !exitf;) {
            SHolder form(State->read());    /* forces proper dealloc */
            exitf = ProcessForm(form);
        }
    }
    /* Exit without error throw means "return to cmd level to wait" */
    catch (DemoErr err) {
        DemoErrmsg(err.message.c_str());
        State.reset();       /* this ends the demo */
    }
}

void DecodeOptions (Sexpr S) {
    for (; S.type == Lisp::tCONS; SPop (S)) {
	Sexpr O = (CAR(S));
	if (O.type != Lisp::ATOM)
	    throw DemoErr ("Non-atomic-symbol option in OPTIONS form.");
        std::string option = O.u.a;
	if (option == "NOXES")
	    State->DontDrawXes = true;
	else if (option == "NOSTOPS")
	    ImplementShowStopPolicy (SHOW_STOPS_NEVER);
	else if (option == "SHOWSTOPS")
	    ImplementShowStopPolicy (SHOW_STOPS_ALWAYS);
	else if (option == "SHOWSTOPSRED")
	    ImplementShowStopPolicy (SHOW_STOPS_RED);
	else if (option == "MAXIMIZE") {
	    ShowWindow (G_mainwindow, SW_SHOWMAXIMIZED);
	    UpdateWindow (G_mainwindow);
	}
	else
            throw DemoErr (FormatString("Unknown Demo option: %s", O.u.a));
    }
}


#ifndef NOTRAINS
void DemoTrain (Sexpr S) {

    if (S.type != Lisp::tCONS || CAR(S).type != Lisp::NUM)
	throw DemoErr ("Missing numeric train number in TRAIN form.");
    int train_no = (int) (CAR(S).u.n);
    SPop(S);

    if (S.type != Lisp::tCONS || CAR(S).type != Lisp::ATOM)
	throw DemoErr ("Missing train command symbol in TRAIN %d form.", train_no);

    std::string cmd = CAR(S).u.a;
    SPop(S);

    if (cmd == "SPEED") {
	if (S.type != Lisp::tCONS || !NUMBERP(CAR(S)))
	    throw DemoErr ("Missing numeric train speed in TRAIN %d SPEED",
			train_no);
	if (!TrainAutoSetSpeed (train_no, *LCoerceToFloat(CAR(S)).u.f))
	    throw DemoErr ("Could not set speed for train# %d", train_no);
    }
    else if (cmd == "CREATE") {
	if (S.type != Lisp::tCONS || CAR(S).type != Lisp::NUM)
	    throw DemoErr ("Missing track # in TRAIN %d CREATE", train_no);

	long start_place_id = CAR(S).u.n;
	long options = 0;
	SPop(S);

	for (; S.type == Lisp::tCONS; SPop(S)) {
	    if (CAR(S).type != Lisp::ATOM)
		throw DemoErr ("Non-atomic create option in train %d CREATE", train_no);

            std::string create_cmd = CAR(S).u.a; // Guaranteed to be upcased.
	    if (create_cmd == "HALTED")
		options |= TRAIN_CTL_HALTED;
	    else if (create_cmd == "HIDEDIALOG")
		options |= TRAIN_CTL_HIDEDLG;
	    else if (create_cmd == "MINIMIZEDIALOG")
		options |= TRAIN_CTL_MINDLG;
	    else if (create_cmd == "FREEWILL")
		options |= TRAIN_CTL_FREEWILL;
	    else {
                std::string complaint (FormatString
                                       ("Train %d CREATE: unknown create option: %s", train_no, create_cmd.c_str()));
                throw DemoErr(complaint);
	    }
	}
	    
	if (!TrainAutoCreate (train_no, start_place_id, options))
	    throw DemoErr ("Could not create train# %d", train_no);
    }
    else {
	int icmd;
	if (!TrainAutoLookupCommand (cmd.c_str(), icmd))  //already upcased
            throw DemoErr (std::string("Unknown TRAIN command: ") + cmd);
        else if (!TrainAutoCmd (train_no, icmd)) {
            std::string complaint(FormatString("Could not TRAIN %d ", train_no) + cmd);
            throw DemoErr (complaint);
        }
    }
}
#endif
    

/* this is an external API  -- see demoapi.h*/
void DemoPause (int haltsw) {
#ifdef NXOLE
    ScriptPause(haltsw);
#endif
    if (State)
        State->ProcessDemoPause(haltsw);
}

void DemoState::ProcessDemoPause(int haltsw) {
    if (haltsw)
        State.reset();
    else {
        if (Paused != NOT_PAUSED) {
            Paused = NOT_PAUSED;
            DemoImpulse(NULL);
        }
        else {
            Paused = PAUSE1;
            DemoSay("Demo pausing...control the interlocking yourself now...");
            DemoTimer(1000);
        }
    }
}

/* this is an external API */
void Demo (const char * fname) {
    if (State){
	DemoErrmsg ("Demo already in progress!");
	return;
    }

    FILE* file = fopen (fname, "r");
    if (file == NULL) {
	usermsgstop ("Can't open demo file %s: %s.", fname, strerror(NULL));
	return;
    }
    TrainMiscCtl (CmKillTrains);

    State = make_unique<DemoState> (file, fname);
    DemoSay ("");
    DemoImpulse (NULL);
}

DemoState::DemoState(FILE* file, const char * fname) {
    File = file;
    RawPath = fname;
    Paused = NOT_PAUSED;
    XWait = NOT_XWAITING;
    DontDrawXes = false;
    ClickTimeMs = CLICK_TIME_DEFAULT_MS;
}

/* this is an external API */
void DemoBlurb (const char * s) {
    DemoSay (s);
    ShowWindow (DemoWindow, SW_SHOW);
    DemoTimer (5000);
    BlurbUp = true;
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
    if (!State) {
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
