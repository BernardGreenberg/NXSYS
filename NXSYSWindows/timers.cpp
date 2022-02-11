#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "timers.h"
#include <vector>

/* Rewritten for dynarray's 7 January 1999, throw out ancient gruffer bowing
   code */
/* Rerewritten for STL vectors 3 September 2014 */

const int CodeBlipMS = 300;
const int FastCodeBlipMS = 120;
static BOOL TimersHalted = FALSE;

class Timer {
    public:
	DWORD Time;
	void* Object;
	NXTimerFn Function;
#ifdef WIN32
	UINT Handle;
#else
	HANDLE Handle;
#endif
	short Valid;
        void Set (void* pobject, NXTimerFn pfn, long ms);
        Timer(void* pobject, NXTimerFn pfn, long ms) {
            Set(pobject, pfn, ms);
        }
        Timer() {
        }
	void Kill ();
	void Mature ();
};

class Coder {
    public:
	void * Object;
	CoderFn Function;
	short Valid;
	void Set (void *, CoderFn);
	void CallIfValid (BOOL phase);
};

static std::vector <Timer> Timers;

class CodersCtl {
    public:
	int BlipMS;			/* must be first */
	int TimerRunning;
	int CodePhase;
	int NActive;
        std::vector <Coder> Coders;

    public:
	CodersCtl(int BlipMS);
	void Reset();
	void TimerFn ();
	void Register(void * object, CoderFn fn);
	void KillOne (void * object);
	void MaybeStartTimer();
};


CodersCtl::CodersCtl (int blip_ms) : Coders(100){
    BlipMS = blip_ms;
    Reset();
}

static CodersCtl StdCoders(CodeBlipMS);
static CodersCtl FastCoders(FastCodeBlipMS);

static void TimeProcI (DWORD time) {
    if (TimersHalted)
	return;
top:
    for (int i = (int)Timers.size()-1; i >= 0; i--) {
	Timer& t = Timers[i];
	if (t.Valid && t.Time <= time) {
	    t.Mature();
	    goto top;
	}
    }
}

void Timer::Mature () {
    /* this order is critical */
    Kill();
    Function(Object);
}

#ifdef WIN32
void CALLBACK TimeProc (HWND, UINT, UINT, DWORD time) {
#else
void CALLBACK _export TimeProc (HWND, UINT, UINT, DWORD time) {
#endif
    TimeProcI(time);
}


void NXTimer (void* object, NXTimerFn fn, long ms) {
    for (size_t i = 0; i < Timers.size(); i++) {
	if (!Timers[i].Valid) {
	    Timers[i].Set (object, fn, ms);
	    return;
	}
    }
    Timer T(object, fn, ms);
    Timers.push_back(T);
}

void Timer::Set (void* object, NXTimerFn fn, long ms) {
    Object = object;
    Function = fn;
    Valid = 1;
	Time = (DWORD)(GetTickCount() + ms);
#ifdef NXSYSMac
    Handle = SetTimer (NULL,  0, (DWORD)ms, (TIMERPROC*) TimeProc);
#else
	Handle = SetTimer(NULL, 0, (DWORD)ms, (TIMERPROC)TimeProc);
#endif
    if (Handle == 0)
	FatalAppExit (0, "Can't get timer.  \"A scarce resource,\" they say. "
		      "Close and/or debug some other apps.");
}

void Timer::Kill () {
    Valid = 0;
    KillTimer (NULL, Handle);
    if (this == &Timers[Timers.size()-1])
	Timers.resize(Timers.size()-1);
}


void KillNXTimers () {
    for (int i = (int)Timers.size()-1; i >= 0; i--)
	if (Timers[i].Valid)
	    Timers[i].Kill();
    Timers.clear();
    StdCoders.Reset();
    FastCoders.Reset();
}

void KillOneTimer (void* data) {
    for (int i = (int)Timers.size()-1; i >= 0; i--)
	if (Timers[i].Valid && Timers[i].Object == data)
	    Timers[i].Kill();
}

/* "Coded" is TA talk for "Flashing electricity" -- the point of the Coder
    system is to make all flashing lights flash in unison as though they
    really were fed from one coded source (and to reduce the number of
    timers, although for switches, it won't). */


void CodersCtl::Reset () {
    Coders.clear();
    TimerRunning = CodePhase = NActive = 0;
}

void CoderTimerFn (void * coderctl) {
    ((CodersCtl*) coderctl)->TimerFn();
}

void CodersCtl::TimerFn () {
    CodePhase ^= 1;
    BOOL p = CodePhase & 1;
    TimerRunning = 0;
    for (size_t i = 0; i < Coders.size(); i++)
	Coders[i].CallIfValid(p);
    if (NActive > 0)
	MaybeStartTimer();
}

void Coder::Set (void * the_object, CoderFn the_fn) {
    Object = the_object;
    Function = the_fn;
    Valid = 1;
}

void Coder::CallIfValid (BOOL phase) {
    if (Valid)
	Function(Object, phase);
}

void CodersCtl::Register (void * object, CoderFn fn) {
    Coder * c = NULL;
    for (size_t i = 0; i < Coders.size(); i++){
	if (!Coders[i].Valid) {
	    c = &Coders[i];
	    break;
	}
    }
    if (c == NULL) {
        Coder coder;
        Coders.push_back(coder);
        c = &(Coders[Coders.size()-1]);
    }
    c->Set (object, fn);
    if (NActive++ == 0) {
	CodePhase = 0;
	MaybeStartTimer();
    }
    c->CallIfValid (CodePhase & 1);
}

void CodersCtl::MaybeStartTimer () {
    if (!TimerRunning) {
	NXTimer (this, CoderTimerFn, BlipMS);
	TimerRunning = 1;
    }
}
	
void NXCoder (void* object, CoderFn fn) {
    StdCoders.Register (object, fn);
}
void NXFastCoder (void* object, CoderFn fn) {
    FastCoders.Register (object, fn);
}


void CodersCtl::KillOne (void* data) {
    for (int i = (int)Coders.size()-1; i >= 0; i--) {
	Coder& C = Coders[i];
	if (C.Valid && C.Object == data){
	    C.Valid = 0;
	    if (i == Coders.size() - 1)
		Coders.resize(Coders.size()-1);
	    NActive--;
	}
    }
    if (NActive <= 0) {
	NActive = 0;			/* I don't know how this happens */
		/* (goes negative) but it does.. 18 January 1998*/
	TimerRunning = 0;
	KillOneTimer (this);
    }
}


void KillOneCoder (void* data) {
    StdCoders.KillOne (data);
}

void KillOneFastCoder (void* data) {	/* hey, not me!!! */
    FastCoders.KillOne (data);
}

void RunTimers () {
    TimersHalted = FALSE;
    TimeProcI ((DWORD) GetTickCount());
}

void HaltTimers() {
    TimersHalted = TRUE;
}
