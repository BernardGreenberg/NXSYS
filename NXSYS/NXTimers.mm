//
//  NXTimers.mm
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/11/14.
//  Copyright (c) 2014 BernardGreenberg. All rights reserved.
//

/* The goal of the last rewrite here, which is to "rewrite this (little) part of NXSYS for the Mac"
 instead of "simulating the Windows APIs", is to take advantage of MAC OS/X (with ARC)'s skill at managing
 pointer lifetimes to extinguish a whole class of bugs and potential bugs in trying to simulate
 Windows' Set/Kill timer, or, worse yet, coding it to second-guess the behavior of NXTimer with a
 shadow/effigy clone.  OS/X will actually go into your structures and invalidate your weak-pointers;
 the tracing in this module that one can enable with TRACE_MAC_TIMERS proves that this is actually happens, for
 timer slots provably get reused, and there is not a line in this program that clears the _MacTimer slot, so
 Somebody has to be doing it.
 
 In fact, tne NXTimer system is a lot more like the Mac timer system than is the Windows timer system, so this
 new version plays to that correspondence and exploits it for safety, robustiness, and simplicity.
 
 The UID system guards against KillTimer not working hard enough or working too hard; maturing timers bear their
 generating UID, and must match exactly for the user action to be called.  Obsolete or redundant firings
 can be dismissed at the gates of this array before then wreak having on possibly deallocated C++ artifacts.
 
 There is nothing to be gained by cleaning these objects up, so let them lie.  When an interlocking is loaded,
 hundreds of automatic signals set timers to drive their stops, growing the array to that number. There is little
 reason in a 64 bit address space to carefully clean them up, esp. since they are reusable.
 */
 
#define TRACE_MAC_TIMERSnot

#ifdef TRACE_MAC_TIMERS
#define MTRACE(x) printf x;
#else
#define MTRACE(x)
#endif

#include <string.h>
#include <stdio.h>
#include <vector>

/* avoid windows.h totally to make sure we don't reference Windows API timer system */

typedef unsigned DWORD;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef void * HWND;
typedef bool BOOLE;  //[sic]
typedef long LPARAM;
typedef int WPARAM;
typedef void TIMERPROC (HWND, UINT, UINT, DWORD);


/* CoderFn in timers.h contains "BOOL", which causes conflict between Windows.h and Cocoa.h regimes. This
 program is 100% Cocoa.h */
typedef void(*CqoderFn)(void*, BOOLE);
#include "timers.h"

DWORD GetTickCount();
static void ResetCoders();

/* Rerewritten for STL vectors 3 September 2014 */
/* Rererewritten for STL of ARC-managed strongpointers 11 Sept 2014 */

static BOOL TimersHalted = FALSE;

static int UID = 10000;
static int getQSize();

@interface NXNSTimer : NSObject
@property DWORD Time;
@property HANDLE Object;
@property NXTimerFn Function;
@property (weak) NSTimer* MacTimer;
@property int slotno;
@property int uid;
@end

static void complainer(void *) {
    assert(!"Supposedly deallocated timer entry called");
}

@implementation NXNSTimer
-(void)Kill
{
    MTRACE(("Kill #%d uid %d %p\n", _slotno, _uid, _MacTimer));
    if (_MacTimer != nil) {
        [_MacTimer invalidate]; // don't need to set ptr null - mac ARC does it for us. trace proves it works.
    }
    _uid = 0;
    _Function = complainer;
}
-(void)Mature:(NSTimer*)timer
{
    int uid = ((NSNumber*)[timer userInfo]).intValue;
    MTRACE(("Mature! #%d, rep uid %d, qsize = %d\n", _slotno, uid, getQSize()));

    if (uid == _uid) {
  // Maturation is supposed to nil the weak pointer, and apparently does. No kill needed.
        _uid = UID++; // this entry will never be hit with this event again.
        _Function(_Object);
        _Object = NULL;
        _Function = complainer;
    } else {
     MTRACE(("!!!Timer #%d UID was wrong, we have %d, mac calls us with %d\n", _slotno, _uid, uid))
    }
}

-(void)Set:(void*)pobject fn:(NXTimerFn)pfn t:(long)ms index:(int)index
{
    CGFloat secs = ((CGFloat)ms)/1000.0;
    _Object = pobject;
    _Function = pfn;
    _Time = (DWORD)(GetTickCount() + ms);
    _slotno = index;
    _uid = UID++;
    _MacTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)secs
                                                 target:self
                                               selector:@selector(Mature:)
                                               userInfo:[NSNumber numberWithInteger:_uid]
                                                repeats:FALSE];
    MTRACE(("Set timer #%d sec %.2f uid %d macp=%p\n", _slotno, secs, _uid, _MacTimer));
}

@end

static std::vector<__strong NXNSTimer*> Timers;
static int getQSize() { return (int)Timers.size();}

static void TimeProcI (DWORD time) {
    MTRACE(("TimeProcI called.\n"));
    if (TimersHalted)
	return;
// this is apparently only called at relay-logic init time.  There is no reason to either
//     a) call maturity handlers
//     b) fool with MacOS
//     c) clear pointers.
//  Simply clobbering the UID's ought invalidate all old meaning and divert pendant firings.

    for (int i = (int)Timers.size()-1; i >= 0; i--) {
	NXNSTimer * t = Timers[i];
	if (t.MacTimer != nil && t.Time <= time) {
            MTRACE(("TimeProcI assassination %d\n", i));
            t.uid = 0;
            t.Function = complainer;
	}
    }

}


void NXTimer (void* object, NXTimerFn fn, long ms) {
    //assert(ms != 6000); what are these?  Stops of automatic signals.
    for (size_t i = 0; i < Timers.size(); i++) {
        if (Timers[i].MacTimer == nil) {
            MTRACE(("Found free slot #%ld uid %d\n", i, Timers[i].uid));
            [Timers[i] Set:object fn:fn t:ms index:(int)i];
            return;
        }
    }
    MTRACE(("New alloc ms %ld size(before)=%d\n",  ms, getQSize()));
    NXNSTimer * t = [[NXNSTimer alloc] init];
    [t Set:object fn:fn t:ms index:getQSize()];
    Timers.push_back(t);
}

void KillNXTimers () {
    for (int i = (int)Timers.size()-1; i >= 0; i--)
        if (Timers[i].MacTimer != nil)
            [Timers[i] Kill];
    //Timers.clear();  // don't do this -- let them lie around
    ResetCoders();
}

/* This is really "purge one Object from the timer system" */
void KillOneTimer (void* object) {
    MTRACE(("Purge object %p\n", object));
    for (int i = (int)Timers.size()-1; i >= 0; i--)
        if (Timers[i].Object == object)
            [Timers[i] Kill];
}

void RunTimers () {
    TimersHalted = FALSE;
    TimeProcI ((DWORD) GetTickCount());
}

void HaltTimers() {
    TimersHalted = TRUE;
}

/* This stuff is unchanged from Windows; maybe it should be in a different file, although
 there continues to be this turf war over "BOOL". */

/* "Coded" is TA talk for "Flashing electricity" -- the point of the Coder
 system is to make all flashing lights flash in unison as though they
 really were fed from one coded source (and to reduce the number of
 timers, although for switches, it won't). */

const int CodeBlipMS = 300;
const int FastCodeBlipMS = 120;

class Coder {
public:
    void * Object;
    CqoderFn Function;
    short Valid;
    void Set (void *, CqoderFn);
    void CallIfValid (BOOLE phase);
};

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
    void Register(void * object, CqoderFn fn);
    void KillOne (void * object);
    void MaybeStartTimer();
};


CodersCtl::CodersCtl (int blip_ms) : Coders(100){
    BlipMS = blip_ms;
    Reset();
}

static CodersCtl StdCoders(CodeBlipMS);
static CodersCtl FastCoders(FastCodeBlipMS);

static void ResetCoders () {
    StdCoders.Reset();
    FastCoders.Reset();
}

void CodersCtl::Reset () {
    Coders.clear();
    TimerRunning = CodePhase = NActive = 0;
}

void CoderTimerFn (void * coderctl) {
    ((CodersCtl*) coderctl)->TimerFn();
}

void CodersCtl::TimerFn () {
    CodePhase ^= 1;
    int p = CodePhase & 1;
    TimerRunning = 0;
    for (size_t i = 0; i < Coders.size(); i++)
        Coders[i].CallIfValid(p);
    if (NActive > 0)
        MaybeStartTimer();
}

void Coder::Set (void * the_object, CqoderFn the_fn) {
    Object = the_object;
    Function = the_fn;
    Valid = 1;
}

void Coder::CallIfValid (BOOLE phase) {
    if (Valid)
        Function(Object, phase);
}

void CodersCtl::Register (void * object, CqoderFn fn) {
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

void NXCoder (void* object, CqoderFn fn) {
    StdCoders.Register (object, fn);
}
void NXFastCoder (void* object, CqoderFn fn) {
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
