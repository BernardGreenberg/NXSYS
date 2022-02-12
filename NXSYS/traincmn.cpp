/* 19 June 1994

   "And God said, let there be trains on the tracks and in all the tunnels
    thereof.  And there were trains."
 
   August 2019, 25 years later.
     Upgraded to explore and exploit C++11 pointer management.
       Relay-based fixed-block color-light signalling is now despised in the
       City of New York, widely blamed for the subway system's ills.

*/

/* To do

   Menu commands: random delivery.
   Accident detection. ?
   GT kludge  ?

*/

#ifdef NXV2
/* 60fps = 41fps */
#define DEFAULT_SPEED 60.0
#else
#define DEFAULT_SPEED 150.0
#endif

#define DEFAULT_TRAIN_LENGTH 610.0
#define DEFAULT_YELLOW_SPEED 60.0

const double HaltSpaceBeforeStopSignal = 40.0;
double CruisingSpeed = DEFAULT_SPEED;
double YellowFeetPerSecond = DEFAULT_YELLOW_SPEED;


#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "commands.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include "nxsysapp.h"
#if NXV2
#include "xtgtrack.h"
#include "signal.h"
#include "dynmenu.h"
#include "NXSYSMinMax.h"

#include "track.h" /// ?? otherwise TrackDef not defined!?!?!

#else
#include "track.h"
#include "signal.h"
#include "stop.h"

#endif

#if NXSYSMac
HWND MacCreateTrainDialog(void* train, int id, bool observant);
void setSliderValue(HWND, int, int);
#endif


#include "traindcl.h"
#include "traindlg.h"
#include "trainapi.h"
#include "timers.h"
#include "compat32.h"
#include "traincfg.h"
#include "trainaut.h"
#include "lyglobal.h"
#include "nxglapi.h"
#include "usermsg.h"
#include "nxproduct.h"
#include <map>
#include <string>
#include "STLExtensions.h"
#include "WinApiSTL.h"

#if NXOGL
const int INTVL_MS = 200;		/* was 5 */
#else
const int INTVL_MS = 500;
#endif

const double INTVL_SECS = ((double)INTVL_MS)/1000.;
#if NXV1
const int IDIST = 3;			/* 100 ft outside xlkg. */  //seemingly unused (says mac compiler)
#endif
const double MIN_ACCEL = -30.0;
const double MAX_ACCEL = 30.0;

extern HICON TrainMinimizedIcon;
extern char app_name[];

BOOL AutoEngageTrains = TRUE;
BOOL AutoLoadNXGL(BOOL barf);

static int CV_TrainID = 0;

static int TrainNoMax = 0;
static void MakeTrainGenId (GraphicObject* tk, int options);

/*
 Protocol -- only the unique_ptr in this map can create OR DELETE the actual trains.  There is no way to get a Train*
 pointer out of it, or any other way.  However, methods of Train can hand "this" to other agents for callback, and this is
 dangerous.  There are three necessary cases now, (1) the NXTimer system for train motion (2) Hooks in signals to prompt
 train motion by signal change (and there were retention bugs here seen in 1996!!) (3) the train dialog itself.
 
 The current protocol (10 Aug 2019) is that all such callbacks must validate the pointers given to them by calling
 ValidateWanderedPointer to check that some train in the map recognizes this pointer as itself (can't ask the map--
 it will refuse to dereference the unique_ptr, which is exactly right)
 
 The dialogs and signals could be changed to shared_ptr's (and the unique_ptr as well), but this is not only overheady,
 but cannot possibly work for the timer system, which must be general enough to accept any pointers.
 
 Trains disappear by calling vanish(), which erases the map entry (or Kill All, which calls map::clear). No one can
 invoke the destructor explicitly (i.e., only STL reference counting may invoke it).
 
 */

static std::map<int, std::unique_ptr<Train> > Trains;

static struct {
    std::string name;
    WPARAM cmd;
} TrainAutoCmds[] {
    {"OBSERVANT", TRD_OBSERVANT},
    {"FREEWILL",  TRD_DEFIANT},
    {"REVERSE",   TRD_REV},
    {"HALT",      TRD_HALT},
    {"KILL",      TRD_KILL},
    {"CALLON",    TRD_COPB},
    {"MINIMIZE",  TRD_MINIMIZE},
    {"HIDE",	  TRD_HIDE},
    {"RESTORE",   TRD_RESTORE}
};

template <class T>
class Popinvect : public std::vector<T> {
public:
    T pop () {
        assert (this->size());
        T result = this->back();
        this->pop_back();
        return result;
    }
};

static Popinvect<int> FreedTrainNumbers;

static void MakeTrainN (int train_no,GraphicObject* tk, int options) {
    assert (train_no != 0);
    assert (Trains.count(train_no) == 0);

    Trains[train_no].reset(new Train(train_no, tk, options));   // how simple with STL
}

static void MakeTrainGenId (GraphicObject* tk, int options) {
    if (FreedTrainNumbers.size())
        MakeTrainN(FreedTrainNumbers.pop(), tk, options);
    else
        MakeTrainN(TrainNoMax + 1, tk, options);
}

Train* Train::IsThisMe(void * v) {
    Train* tv = reinterpret_cast<Train*>(v);
    if (tv == this)
        return tv;
    else
        return nullptr;
}

Train* ValidateWanderedPointer(void* vp, bool allow_null=FALSE) {
    for (auto& pair : Trains)
        if (Train * train = pair.second->IsThisMe(vp))
            return train;
    assert (allow_null && "Pointer handed back to train system is not a known train.");
    return nullptr;
}

DLGPROC_DCL Train_DlgProc (HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
    BOOL trs;
    int train_no;
    
  switch (message) {
      case WM_INITDIALOG:
#if ! NXOGL
          ShowWindow (GetDlgItem (dialog, TRD_CABVIEW), SW_HIDE);
#endif
          ShowWindow (GetDlgItem (dialog, TRD_COPB), SW_HIDE);
      return TRUE;
    case WM_VSCROLL:
          train_no = GetDlgItemInt(dialog, TRD_TRAIN_ID, &trs, FALSE);
          
#if WIN32
          return Trains[train_no]->ScrollHandler (LOWORD(wParam), HIWORD(wParam));
#else	
          return Trains[train_no]->ScrollHandler (wParam, LOWORD (lParam));
#endif
      case WM_COMMAND:
          train_no = GetDlgItemInt(dialog, TRD_TRAIN_ID, &trs, FALSE);
          if (wParam == IDCANCEL) {
              ShowWindow (dialog, SW_SHOWMINIMIZED);
              return TRUE;
          }
          else return Trains[train_no]->Command(wParam);
      case WM_CLOSE:
          ShowWindow (dialog, SW_SHOWMINIMIZED);
          return TRUE;
      case WM_PAINT:
          if (IsIconic (dialog)) {
              PAINTSTRUCT ps;
              HDC dc = BeginPaint (dialog, &ps);
              DrawIcon (dc, 0, 0, TrainMinimizedIcon);
              EndPaint (dialog, &ps);
              return TRUE;
          }
          else return FALSE;
      default:
          return FALSE;
    }
}

void NBDSetWindowText (HWND window, char* text);


void Train::StringFldF(int id, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    StringFld(id, FormatStringVA(fmt, args));
}

void Train::StringFld (int id, const char* text) {
    NBDSetWindowText (GetDlgItem (Dialog, id), text);
}

void Train::StringFld(int id, const std::string& str) {
    StringFld(id, str.c_str());
}

void Train::ReportSig (Signal* g, int nameid, int stateid) {

    bool call_on = false;
    if (g == NULL) {
	StringFld (nameid, "None");
	StringFld (stateid, "");
    }
    else {
	StringFld (nameid, g->CompactName());
	g->ComputeState();
        std::string aspect;

        for (auto& head : g->Heads) {
            if (head.State != 'X') {
                if (aspect.size())
                    aspect += ' ';
                aspect += head.State;
	    }
	    if (head.height == 1 && head.Lights[0] == 'Y')
		call_on = (head.State == 'Y');
	}
	StringFld (stateid, aspect);
    }
    if (NextSig != NULL && NextSig->TStop != NULL && !NextSig->TStop->Tripping)
	call_on = false;

    if (call_on != CODisplay) {
	CODisplay = call_on;
	ShowWindow (GetDlgItem (Dialog, TRD_COPB), call_on ? SW_SHOW  : SW_HIDE);
    }
}
    

void Train::UpdateSwitches () {
    SendDlgItemMessage (Dialog, TRD_OBSERVANT, BM_SETCHECK, observant, 0L);
    SendDlgItemMessage (Dialog, TRD_DEFIANT, BM_SETCHECK, !observant, 0L);
}

void Train::UpdatePositionReport () {

#if NXV1
    double x = front.x;
    int ix = (int)x;
    StringFldF(TRD_LOC, "%c%d-%d+%02d",
               front.td->Route, front.td->TrackNo,
               ix,
               (int) (100.0 * (x  - (double) ix)));
#else
    StringFldF(TRD_LOC, "%s+%02d", front.LastIJID,
	     (int)(100.0*front.FeetSinceLastIJ));
#endif

#if NXSYSMac
    double fraction = Speed/Cruise;
    double of100 = 100.0 * fraction;
    setSliderValue(Dialog, TRD_MC, (int)of100);
#else
    HWND Scrollbar = GetDlgItem (Dialog, TRD_MC);
    int scrop = (int)(Cruise-Speed);
    if (GetScrollPos (Scrollbar, SB_CTL) != scrop) {
	SetScrollPos (Scrollbar, SB_CTL, scrop, FALSE);
	InvalidateRect (Scrollbar, NULL, 0);
 }
#endif
    StringFldF (TRD_SPEED, "%d/%d", int(Speed), int (Cruise));
    ReportSig (NextSig, TRD_NEXT_SIG_NAME, TRD_NEXT_SIG_STATE);
#if NXOGL
    if (CV_TrainID == id)
	NXGLUpdateTrainPos (id, front.td, (long)(100.0*front.x), southp);
#endif
}

Train::Train (int train_no, GraphicObject * g, int options) {

    id = train_no;
    TrainNoMax = NXMAX(TrainNoMax, train_no);
    observant = ((options & TRAIN_CTL_HALTED) == 0);
   
    TimerPending = false;
#if NXSYSMac
    Dialog = MacCreateTrainDialog(this, id, observant);  // VIOLATING unique_ptr protocol
#else
    DLGPROC dd = reinterpret_cast<DLGPROC>(&Train_DlgProc);
    Dialog = CreateDialog (app_instance, "Train", G_mainwindow, dd);
#endif
#if NXV1
    TrackTime = 0;
#endif

    Length = Glb.TrainLengthFeet;
    Cruise = CruisingSpeed;

    Speed =  observant ? Cruise : 0.0;
    LastTargetSpeed = Speed;
    CODisplay = false;
    TrackUnit * ts = (TrackUnit *) g;
    X_Of_Next_Signal = 0.0;
    InitPositionTracking(ts);
    NextSig = NULL;
    SetWindowTextS (Dialog, FormatString("#%d Train Control", id));
    Time = GetTickCount();
    StringFld(TRD_TRAIN_ID, std::to_string(id));
    StringFld(TRD_LENGTH, std::to_string((int)Length));
#if ! NXSYSMac
    SetScrollRange (GetDlgItem (Dialog, TRD_MC), SB_CTL,0, int(Cruise), FALSE);
#endif
    InitPosition();
    ComputeWindowPlacement();
#if NXOGL
    if (Trains.size() == 1 && AutoEngageTrains && AutoLoadNXGL(FALSE))
	CV_TrainID = id;
    SetCabviewCheck (CV_TrainID == id);
#endif
    int show = SW_SHOWNORMAL;
    if (options & TRAIN_CTL_MINDLG)
	show = SW_SHOWMINIMIZED;
    else if (options & TRAIN_CTL_HIDEDLG)
	show = SW_HIDE;
    ShowWindow (Dialog, show);
    if (show != SW_SHOWNORMAL)
	SetFocus (G_mainwindow);
}

#if NXOGL
void Train::SetCabviewCheck (BOOL v) {
    SendMessage(GetDlgItem (Dialog, TRD_CABVIEW),BM_SETCHECK,
		(WPARAM)v, 0);
}
#endif

#if NXV1         //v2 is in xtrains.cpp
void Train::InitPositionTracking (TrackUnit * ts) {
    southp = ts->NominalSouthp;
    front.td = ts->Track;
    front.ts = NULL;
    front.x = southp ? front.td->LastSno + IDIST
	      : front.td->Station_base - IDIST;
    back.td = front.td;
    back.ts = NULL;
}
#endif

void Train::InitPosition () {

    if (NextSig != NULL)
	NextSig->Hook (NULL);
    NextSig = NULL;
    InstallNextSig();
#if NXV1
    StringFld (TRD_NORTHSOUTH, (southp ^ Glb.RightIsSouth)
	       ? "Southbound" : "Northbound");
#endif
    StringFld (TRD_LAST_SIG_NAME, "None");
    StringFld (TRD_LAST_SIG_STATE, "");

    UpdateSwitches();
    UpdatePositionReport();
    ComputeNextMotion();
    CheckHalted();

}

void TrainDialog (GraphicObject* g, int haltctl) {

#if NXV2
    if (!VerifyTrackSelectionAcceptability ((TrackUnit *) g))
	return;
#endif
    
    MakeTrainGenId (g, (haltctl & TRAIN_HALTCTL_HALTED) ? TRAIN_CTL_HALTED : 0);
}

void Train::Observance () {
    double target_speed = 0.0; /* placate Mac logic analyzer */
    double s;

    /* identity of next sig must already be right*/
    if (NextSig == NULL)
	target_speed = LastTargetSpeed;
    else
	switch (NextSig->ComputeState()) {
	    case 'G':
		target_speed = Cruise;
		break;
	    case 'Y':
		target_speed = YellowFeetPerSecond;
		break;
	    case 'R':
		target_speed = 0;
		break;
	    default:
		FatalAppExit (0, "No signal state at train Observance time?");
	}
    LastTargetSpeed = target_speed;
    if (target_speed == Speed)
	return;

    double next_loc;

#if NXV1
    if (NextSig == NULL)
	if (southp)
	    next_loc = (int)(front.td->Station_base -(Length+99.0)/100.- 2);
	else
	    next_loc = (int)(front.td->LastSno + (Length+99.0)/100.+ 2);
    else
	next_loc = NextSig->RealStationPos;

    if (southp)
	s = (front.x - next_loc)*100.0;
    else
	s = (next_loc - front.x)*100.0;
#endif

#if NXV2
    if (NextSig == NULL)
	next_loc = front.x + 100.0;
    else
	next_loc = X_Of_Next_Signal;

    s = (next_loc - front.x)*100.0;
#endif

    if (target_speed == 0.0)
	s -= HaltSpaceBeforeStopSignal;

    if (s < 0.0) {
	SetSpeed (0.0);
	return;
    }

    double accel = (target_speed*target_speed - Speed*Speed)/(2.0*s);
    /* in true feet per second per second */
    if (accel > MAX_ACCEL) accel = MAX_ACCEL;
    else if (accel < MIN_ACCEL) accel = MIN_ACCEL;
    double new_speed = Speed + accel*INTVL_SECS;

    if (new_speed < 1.5)
	new_speed = 0;
    
    if (new_speed != Speed)
	SetSpeed (new_speed);

}

void Train::ComputeNextMotion() {
    /*This method will DELETE THIS TRAIN when tripped */
    long now = GetTickCount();
    /* overflow 32 seconds?  in debugger?*/

    int interval = (int)(now - Time);
    double move = ((double)interval*(Speed/100.0))/1000.0; /* ms, not seconds */
    double flength = (double) Length/100.0;
#if NXV1
    if (southp) {
	front.x -= move;
	back.x = front.x + flength;
    }
    else {
#endif
	front.x += move;
	back.x = front.x - flength;
#if NXV1
    }
#endif
    Time = now;
    int tripstop =
      NextSig!= NULL && NextSig->TStop != NULL && NextSig->TStop->Tripping;
    Signal * was_next_sig =  NextSig;
    InstallNextSig ();
    if (!ComputeOccupations()) {
        vanish();
	return;
    }
    if (observant)
	Observance();
    UpdatePositionReport ();
    if (was_next_sig != NextSig && tripstop)
	if (!(was_next_sig->AK_p () && Speed < 5.0)) {
	    Trip (was_next_sig);
	    return;			/* deleted THIS */
	}
    if (Speed > 0.0)
	if (!TimerPending) {
	    TimerPending = true;
	    NXTimer (this, StaticTimerHandler, INTVL_MS);  //VIOLATES unique_ptr with permission
	}
}

void Train::Trip (Signal * g) {
    observant = false;
    UpdateSwitches();
    SetSpeed (0.0);
    std::string msg(FormatString("Train #%d has overrun signal %s and been tripped "
                                 "by its automatic train stop. This train will be destroyed.",
                                 id, g->CompactName().c_str()));
#if NXSYSMac
    MessageBoxWithImage (0, msg.c_str(), PRODUCT_NAME " Train Manager", "TrainWreck256.png", MB_OK|MB_ICONSTOP);
#else
    MessageBox (0, msg.c_str(), PRODUCT_NAME " Train Manager", MB_OK|MB_ICONSTOP);
#endif
    vanish();
}


void TrainSigHook (Signal * g, void * v) {

    Train * t = ValidateWanderedPointer(v, TRUE);
    if (t == nullptr || !t->CheckNextSigEQ(g)) {
        /* I don't know how this happens (hook gets left over), but try to program
         around it to prevent blowouts until debugged. 11 December 1996
         
         22 and some years later, "We still don't really know...." 10 Aug 2019
         */
        g->Hook(NULL);
    } else
        t->MaybeNoticeSignalChange(g);
}

void Train::MaybeNoticeSignalChange (Signal * g) {

    if (FindNextSig() != g)    // The great void (as it were) is unsignalled ...
	return;

    ReportSig (g, TRD_NEXT_SIG_NAME, TRD_NEXT_SIG_STATE);
    if (observant)
	ComputeNextMotion();		/* MUST BE LAST CALL */
}

void Train::InstallNextSig () {
    Signal * g = FindNextSig ();
    if (g != NextSig) {
	StringFld (TRD_LAST_SIG_NAME, GetDlgItemText(Dialog, TRD_NEXT_SIG_NAME));
	StringFld (TRD_LAST_SIG_STATE, GetDlgItemText(Dialog, TRD_NEXT_SIG_STATE));

	if (g != NULL)
            g->Hook(this);    // Moby VIOLATES unique_ptr protocol.  Callback calls ValidateWanderedPointer.
	if (NextSig != NULL)
	    NextSig->Hook (NULL);

	NextSig = g;
    }
}

void Train::StaticTimerHandler (void * v) {
    ValidateWanderedPointer(v)->TimerHandler();
}

void Train::TimerHandler(){
    assert(TimerPending && "Timer handler called with no apparent timer pending on this train.");
    TimerPending = false;
    ComputeNextMotion();  //Can vanish the Train
}

#if NXV2
void Train::SetUnoccupied (TrackUnit * ts) {
    if (Occupied.count(ts)) {
        Occupied.erase(ts);
        ts->DecrementTrainOccupation();
    }
}
#endif

void Train::SetOccupied (TrackUnit * ts) {
    if (Occupied.count(front.ts))
        return;
#if NXV1
    OcTrackTime[i] = ++TrackTime;
#endif
    Occupied.insert(ts);
    ts->IncrementTrainOccupation();   //Will not work in V1;  to hell with V1.
}

void Train::vanish() {
    assert (Trains.count(id));
    Trains.erase(id);
}

Train::~Train () {
    if (CV_TrainID == id)
	CV_TrainID = 0;
    for (auto ts : Occupied)
        ts->DecrementTrainOccupation();
    if (NextSig != NULL)
	NextSig->Hook (NULL);
    KillTimer();
    DestroyWindow (Dialog);
    FreedTrainNumbers.push_back(id);
    //Don't erase from STL map!  that's the only way we could have gotten here!
}

void Train::KillTimer() {
    KillOneTimer (this);
    TimerPending = false;
}


void Train::Reverse (){
#if NXV1
    southp = !southp;
#endif
    std::swap<Pointpos>(front, back);
#if NXV2
    front.IAmFront = TRUE;
    back.IAmFront = FALSE;
    front.Reverse(0.0f);
    back.Reverse(front.x - Length/100.0);
#endif
    InitPosition();
#if NXV2
    if (NextSig && NextSig->XlkgNo != 0)
	TrySignalIDBox (NextSig->XlkgNo);
#endif
}

int Train::Command (WPARAM cmd) {

    switch (cmd) {
	case TRD_HALT:
	    observant = false;
	    UpdateSwitches();
	    SetSpeed(0.0);
	    return TRUE;
	case TRD_REV:
	    Reverse();
	    return TRUE;
	case TRD_KILL:
            vanish();
	    return TRUE;
	case TRD_OBSERVANT:
	    observant = true;
	    UpdateSwitches();
	    ComputeNextMotion();	/* MUST BE LAST CALL */
	    return TRUE;
	case TRD_DEFIANT:
	    observant = false;
	    UpdateSwitches();
	    ComputeNextMotion();	/* MUST BE LAST CALL */
	    return TRUE;
	case TRD_COPB:
	    if (NextSig != NULL && NextSig->TStop != NULL) {
		NextSig->TStop->PressStopPB();
                ComputeNextMotion(); // 9-9-2014
            }
	    return TRUE;
	case TRD_MINIMIZE:
	    if (Dialog) ShowWindow(Dialog, SW_MINIMIZE);
	    return TRUE;
	case TRD_RESTORE:
	    if (Dialog) ShowWindow(Dialog, SW_RESTORE);
	    return TRUE;
#if NXOGL
	case TRD_CABVIEW:
	{
	    BOOL newstate = !(CV_TrainID == id);
	    if (newstate) {
		if (!AutoLoadNXGL(TRUE))
		    return TRUE;
		if (CV_TrainID != 0 && Trains.count(CV_TrainID))
		    Trains[CV_TrainID]->SetCabviewCheck(FALSE);
		CV_TrainID = id;
		UpdatePositionReport();
	    }
	    else CV_TrainID = 0;
	    SetCabviewCheck (newstate);
	}
#endif
	default: 
	    return FALSE;
    }
}

#if NXSYSMac
void sendTrainSlider(void*v, int of100) {
    ValidateWanderedPointer(v)->SetSpeedFractional(of100/100.0);
}

void sendTrainCommand(void* v, int cmd) {
    ValidateWanderedPointer(v)->Command((WPARAM)cmd);
}
#endif

void Train::SetSpeedFractional(double fraction) {
    ExplicitSpeed(fraction * Cruise);
}

void Train::ExplicitSpeed (double speed) {
    
    double was_speed = Speed;

    speed =NXMIN(Cruise, NXMAX(0.0, speed));

    if (speed < Cruise/100.0)
	speed = 0.0;

    Speed = speed;

    UpdatePositionReport();

    CheckHalted();

    if (Speed == 0.0 && was_speed != 0.0)
	KillTimer();
    if (was_speed == 0.0 && Speed != 0.0)
	ComputeNextMotion();		/* must be LAST CALL */
}


int Train::ScrollHandler (WPARAM scrollcode, WORD nPos) {
    double speed = Speed;
    if (scrollcode == SB_THUMBPOSITION || scrollcode == SB_THUMBTRACK)
	speed = (int) Cruise - nPos;
    else if (scrollcode == SB_LINEDOWN)
	speed *= .9;
    else if (scrollcode == SB_LINEUP)
	if (speed < Cruise/12)
	    speed = Cruise/11;
	else
	    speed *= 1.1;
    else return FALSE;

    /* no minimum */
    if (speed > Cruise)
	speed = Cruise;
    ExplicitSpeed (speed);
    return TRUE;
}

BOOL  Train::ShowWindowIfOwnTU (TrackUnit * tu) {
    if (Occupied.count(tu)) {
        ShowWindow(Dialog, SW_RESTORE);
        SetFocus(Dialog);
        return TRUE;
    }
    return FALSE;
}

void Train::SetSpeed (double spd) {
    if (fabs(spd) < 1.00)
	spd = 0.0;
    Speed = spd;
    if (Speed == 0.0)
	KillTimer();
    CheckHalted();
    UpdatePositionReport();
}

void Train::CheckHalted() {
#if! NXSYSMac // maybe pretty up later
    HWND reverser = GetDlgItem (Dialog, TRD_REV);
    EnableWindow (reverser, (Speed == 0.0));
#endif
}

void TrainMiscCtl (int cmd) {
    if (cmd == CmKillTrains) {
        // This properly blew up when the train destructor falsely called erase.
        Trains.clear();
    }
    else {
        for (auto& it : Trains) {
            switch (cmd) {
                case CmHaltTrains:
                    it.second->SetSpeed (0.0);
                    break;
                case CmHideTrainWindows:
                    ShowWindow (it.second->Dialog, SW_SHOWMINIMIZED);
                    break;
                case CmShowTrainWindows:
                    ShowWindow (it.second->Dialog, SW_SHOWNORMAL);
                    break;
                default:
                    break;
            }
        }
    }
}

static SC_cord X;

void Train::ComputeWindowPlacement() {
    /* Effective no-op on the Mac right now.  MoveWindow is a stub, and GetWindowRect
       doesn't work on the Desktop window well, or perhaps ever ... */
    RECT rc;
    GetWindowRect (GetDesktopWindow(), &rc);
    int dtw = rc.right-rc.left;
    int dth = rc.bottom-rc.top;
    RECT wrc;
    GetWindowRect (Dialog, &wrc);
    int dlw = wrc.right-wrc.left;
    int dlh = wrc.bottom-wrc.top;
    if (Trains.size() == 1)
	X = (int)(.4*dlw);
    else {
	X += (int)(.6*dlw);
	if (X+dlw >= dtw)
	    X = (int)(.2*dlw);
    }
    MoveWindow (Dialog, X, (int)(dth - 1.25*dlh), dlw, dlh, FALSE);
}

#if NXV1
TrackUnit * ChooseTrackNumber (int tn) {
    for (int i = 0; i < Glb.TrackDefCount; i++)
	if (TrackDefs[i]->TrackNo == tn)
	    return TrackDefs[i]->First;
    return NULL;
}
#endif

void SetTrainKinematicsDefaults() {
     CruisingSpeed = DEFAULT_SPEED;
     YellowFeetPerSecond = DEFAULT_YELLOW_SPEED;
     Glb.TrainLengthFeet = DEFAULT_TRAIN_LENGTH;
}


/* Automation! but not OLE! -- 29 November 1996 */


int TrainAutoCreate   (int train_no, long track_id_no, long options) {

/* maybe create by track section number, find loose end? */
    TrackUnit * tk;
#if NXV1
    const char * what = "track number";
    tk = ChooseTrackNumber (track_id_no);
#endif
#if NXV2
    const char * what = "track end IJ #";
    tk = FindTrainEntryTrackSectionByNomenclature (track_id_no);
#endif
    if (!tk) {
	usermsgstop ("Invalid train starting %s in demo form: %ld", what, track_id_no);
	return 0;
    }
    if (Trains.count(train_no)) {
        usermsgstop ("Train number already in use: %d", train_no);
        return 0;
    }
    MakeTrainN (train_no, tk, int(options));
    return train_no;
}

BOOL TrainAutoValidateTrainNo (int train_no) {
    return Trains.count(train_no) > 0;
}

BOOL TrainAutoCmd (int train_no, WPARAM cmd) {
    return TrainAutoValidateTrainNo(train_no)
	    && Trains.at(train_no)->Command (cmd);
}

BOOL TrainAutoSetSpeed (int train_no, double speed) {
    if (!TrainAutoValidateTrainNo (train_no))
	return NULL;
    Trains.at(train_no)->ExplicitSpeed (speed);
    return TRUE;
}

double TrainAutoGetSpeed (int train_no) {
    if (!TrainAutoValidateTrainNo(train_no))
	return 0.0;
    else return Trains.at(train_no)->GetSpeed();
}

BOOL TrainAutoLookupCommand (const char * scmd, int&icmd) {
    for (auto& command : TrainAutoCmds) {
        if (command.name == scmd) { //must already be upcased.
	    icmd = command.cmd;
	    return TRUE;
	}
    }
    return FALSE;
}

BOOL WindowToTopFromTrackUnit (TrackUnit * tu) {
    for (auto& pair : Trains) {
        if (pair.second->ShowWindowIfOwnTU (tu))
            return TRUE;
    }
    return FALSE;
}
#if WINDOWS

extern HWND ChooseTrackDlg;

int FilterTrainDialogMessages(MSG* mp) {
    if (ChooseTrackDlg != NULL)
        if (IsDialogMessage(ChooseTrackDlg, mp))
            return 1;
    for (decltype(Trains.begin()) it = Trains.begin(); it != Trains.end(); it++) {
        if (IsDialogMessage((it->second)->Dialog, mp))
            return 1;
    }
    return 0;
}
#endif