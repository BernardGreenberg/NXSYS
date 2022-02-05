/* 19 June 1994

   "And God said, let there be trains on the tracks and in all the tunnels
    thereof.  And there were trains."

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

#include "nxsysapp.h"
#ifdef NXV2
#include "xtgtrack.h"
#include "signal.h"
#include "dynmenu.h"

#include "track.h" /// ?? otherwise TrackDef not defined!?!?!

#else
#include "track.h"
#include "signal.h"
#include "stop.h"

#endif

#ifdef NXSYSMac
HWND MacCreateTrainDialog(void* train, int id, bool observant);
void setSliderValue(HWND, int, int);
#else
#define stricmp _stricmp
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

#ifdef NXOGL
const int INTVL_MS = 200;		/* was 5 */
#else
const int INTVL_MS = 500;
#endif

const double INTVL_SECS = ((double)INTVL_MS)/1000.;
#ifdef NXV1
const int IDIST = 3;			/* 100 ft outside xlkg. */  //seemingly unused (says mac compiler)
#endif
const double MIN_ACCEL = -30.0;
const double MAX_ACCEL = 30.0;

extern HICON TrainMinimizedIcon;
extern char app_name[];
extern TrackDef *TrackDefs[];

BOOL AutoEngageTrains = TRUE;
BOOL AutoLoadNXGL(BOOL barf);

static int CV_TrainID = 0;

static int TrainsAlive = 0;
static int TrainNoMax = 0;
static Train* MakeTrain ();

static std::map<int,Train*> Trains;

static struct {
    const char * name;
    WPARAM cmd;
} TrainAutoCmds [] = {
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

static int N_TrainAutoCmds = sizeof(TrainAutoCmds)/sizeof(TrainAutoCmds[0]);

static Train* MakeTrainN (int train_no) {
    if (train_no == 0)
	return MakeTrain();
    if (Trains.count(train_no) != 0)
        return NULL;
    Train * t = new Train;
    if (t == NULL) // can this ever really happen!?
	return NULL;
    t->id = train_no;
    Trains[train_no] = t;   // how simple with STL!
    if (train_no > TrainNoMax)
        TrainNoMax = train_no;
    TrainsAlive++;
    return t;
}

static Train* MakeTrain () {
    for (int i = 1; i <= TrainNoMax; i++)
        if (Trains.count(i) == 0)
            return MakeTrainN(i);
    return MakeTrainN(TrainNoMax + 1);
}


DLGPROC_DCL Train_DlgProc (HWND dialog, unsigned message, WPARAM wParam, LPARAM lParam)
{
  BOOL trs;
  switch (message) {
    case WM_INITDIALOG:
#ifndef NXOGL
      ShowWindow (GetDlgItem (dialog, TRD_CABVIEW), SW_HIDE);
#endif
      ShowWindow (GetDlgItem (dialog, TRD_COPB), SW_HIDE);
      return TRUE;
    case WM_VSCROLL:
	return Trains[GetDlgItemInt(dialog, TRD_TRAIN_ID, &trs, FALSE)]->
#ifdef WIN32
		ScrollHandler (LOWORD(wParam), HIWORD(wParam));
#else	
		ScrollHandler (wParam, LOWORD (lParam));
#endif
    case WM_COMMAND:
      if (wParam == IDCANCEL) {
	  ShowWindow (dialog, SW_SHOWMINIMIZED);
	  return TRUE;
      }
      else return Trains[GetDlgItemInt(dialog, TRD_TRAIN_ID, &trs, FALSE)]
	      ->Command(wParam);
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

void Train::StringFld (int id, const char* text) {
    NBDSetWindowText (GetDlgItem (Dialog, id), text);
}

void Train::ReportSig (Signal* g, int nameid, int stateid) {
    char buf[20];
    int cof = 0;
    if (g == NULL) {
	StringFld (nameid, "None");
	StringFld (stateid, "");
    }
    else {
	StringFld (nameid, g->Name());
	char* b = buf;
	g->ComputeState();
	for (int i = 0; i < g->HeadCount; i++) {
	    SigHead * h = g->Heads[i];
	    char state = h->State;
	    if (state != 'X') {
		if (b != buf) *b++ = ' ';
		*b++ = h->State;
	    }
	    if (i > 1 && h->height == 1 && h->Lights[0] == 'Y')
		cof = (h->State == 'Y');
	}
	*b++ = '\0';
	StringFld (stateid, buf);
    }
    if (NextSig != NULL && NextSig->TStop != NULL
	&& !NextSig->TStop->Tripping)
	cof = 0;
    if (cof != CODisplay) {
	CODisplay = cof;
	ShowWindow (GetDlgItem (Dialog, TRD_COPB), cof ? SW_SHOW  : SW_HIDE);
    }
}
    
void Train::DecFld (int fld, int val) {
    char buf[10];
    sprintf (buf, "%d", val);
    StringFld (fld, buf);
}


void Train::UpdateSwitches () {
    SendDlgItemMessage (Dialog, TRD_OBSERVANT, BM_SETCHECK, observant, 0L);
    SendDlgItemMessage (Dialog, TRD_DEFIANT, BM_SETCHECK, !observant, 0L);
}

void Train::UpdatePositionReport () {

    char buf[30];
#ifdef NXV1
    double x = front.x;
    int ix = (int)x;
    sprintf (buf, "%c%d-%d+%02d",
	     front.td->Route, front.td->TrackNo,
	     ix,
	     (int) (100.0 * (x  - (double) ix)));
#else
    sprintf (buf, "%s+%02d", front.LastIJID,
	     (int)(100.0*front.FeetSinceLastIJ));
#endif
    StringFld (TRD_LOC, buf);
    sprintf (buf, "%d/%d", int(Speed), int (Cruise));
#ifdef NXSYSMac
    double fraction = Speed/Cruise;
    double of100 = 100.0 * fraction;
    setSliderValue(Dialog, TRD_MC, (int)of100);
#else
    HWND Scrollbar = GetDlgItem (Dialog, TRD_MC);
    int scrop = (int) (Cruise-Speed);
    if (GetScrollPos (Scrollbar, SB_CTL) != scrop) {
	SetScrollPos (Scrollbar, SB_CTL, scrop, FALSE);
	InvalidateRect (Scrollbar, NULL, 0);
 }
#endif
 StringFld (TRD_SPEED, buf);
    ReportSig (NextSig, TRD_NEXT_SIG_NAME, TRD_NEXT_SIG_STATE);
#ifdef NXOGL
    if (CV_TrainID == id)
	NXGLUpdateTrainPos (id, front.td, (long)(100.0*front.x), southp);
#endif
}

void Train::Init (GraphicObject * g, long options) {

    observant = ((options & TRAIN_CTL_HALTED) == 0);
   
    TimerPending = 0;
#ifdef NXSYSMac
    Dialog = MacCreateTrainDialog(this, id, (observant == 0) ? false : true);
#else
    DLGPROC dd = reinterpret_cast<DLGPROC>(&Train_DlgProc);
    Dialog = CreateDialog (app_instance, "Train", G_mainwindow, dd);
#endif
#ifdef NXV1
    TrackTime = 0;
#endif

    Length = Glb.TrainLengthFeet;
    Cruise = CruisingSpeed;

    Speed =  observant ? Cruise : 0.0;
    LastTargetSpeed = Speed;
    NOccupied = 0;
    CODisplay = 0;
    TrackUnit * ts = (TrackUnit *) g;
    X_Of_Next_Signal = 0.0;
    InitPositionTracking(ts);
    char buf[40];
    NextSig = NULL;
    sprintf (buf, "#%d Train Control", id);
    SetWindowText (Dialog, buf);
    Time = GetTickCount();
    DecFld (TRD_TRAIN_ID, id);
    DecFld (TRD_LENGTH, (int) Length);
#ifndef NXSYSMac
    SetScrollRange (GetDlgItem (Dialog, TRD_MC), SB_CTL,0, int(Cruise), FALSE);
#endif
    InitPosition();
    ComputeWindowPlacement();
#ifdef NXOGL
    if (TrainsAlive == 1 && AutoEngageTrains && AutoLoadNXGL(FALSE))
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

#ifdef NXOGL
void Train::SetCabviewCheck (BOOL v) {
    SendMessage(GetDlgItem (Dialog, TRD_CABVIEW),BM_SETCHECK,
		(WPARAM)v, 0);
}
#endif

#ifdef NXV1
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
#ifdef NXV1
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

#ifdef NXV2
    if (!VerifyTrackSelectionAcceptability ((TrackUnit *) g))
	return;
#endif

    Train* t = MakeTrain ();
    if (t == NULL)
	usermsg ("Too many trains");
    else
	t->Init(g, (haltctl & TRAIN_HALTCTL_HALTED) ? TRAIN_CTL_HALTED : 0);
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

#ifdef NXV1
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

#ifdef NXV2
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
    /*This method can delete THIS */
    long now = GetTickCount();
    /* overflow 32 seconds?  in debugger?*/

    int interval = (int)(now - Time);
    double move = ((double)interval*(Speed/100.0))/1000.0; /* ms, not seconds */
    double flength = (double) Length/100.0;
#ifdef NXV1
    if (southp) {
	front.x -= move;
	back.x = front.x + flength;
    }
    else {
#endif
	front.x += move;
	back.x = front.x - flength;
#ifdef NXV1
    }
#endif
    Time = now;
    int tripstop =
      NextSig!= NULL && NextSig->TStop != NULL && NextSig->TStop->Tripping;
    Signal * was_next_sig =  NextSig;
    InstallNextSig ();
    if (!ComputeOccupations()) {
	delete this;
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
	    TimerPending = 1;
	    NXTimer (this, StaticTimerHandler, INTVL_MS);
	}
}

void Train::Trip (Signal * g) {
    char buf [200];
    observant = 0;
    UpdateSwitches();
    SetSpeed (0.0);
    sprintf (buf, "Train #%d has illegally passed signal %s and been tripped "
	     "by its automatic train stop. This train will be killed.",
	     id, g->Name());
    MessageBox (0, buf, PRODUCT_NAME " Train Manager", MB_OK|MB_ICONSTOP);
    delete this;
}


void TrainSigHook (Signal * g, void * v) {
    ((Train *) v)->MaybeNoticeSignalChange(g);
}

void Train::MaybeNoticeSignalChange (Signal * g) {
    /* I don't know how this happens (hook gets left over, but try to program
       around it to prevent blowouts until debugged. 11 December 1996 */
    if (Trains.count(id) == 0 || Trains[id] != this || NextSig != g) {
	g->Hook(NULL);
	return;
    }
    if (FindNextSig() != g)
	return;
    ReportSig (g, TRD_NEXT_SIG_NAME, TRD_NEXT_SIG_STATE);
    if (observant)
	ComputeNextMotion();		/* MUST BE LAST CALL */
}

void Train::InstallNextSig () {
    Signal * g = FindNextSig ();
    if (g != NextSig) {
	char buf[30];
	GetDlgItemText (Dialog, TRD_NEXT_SIG_NAME, buf, sizeof(buf)-1);
	StringFld (TRD_LAST_SIG_NAME, buf);
	GetDlgItemText (Dialog, TRD_NEXT_SIG_STATE, buf, sizeof(buf)-1);
	StringFld (TRD_LAST_SIG_STATE, buf);

	if (g != NULL)
	    g->Hook(this);
	if (NextSig != NULL)
	    NextSig->Hook (NULL);

	NextSig = g;
    }
}


void Train::StaticTimerHandler (void * v) {
    ((Train *) v)->TimerPending = 0;
    ((Train *) v)->ComputeNextMotion();	/* MUST BE LAST CALL */
}


#ifdef NXV2
void Train::SetUnoccupied (TrackUnit * ts) {
    for (int i = 0; i < NOccupied; i++)
	if (Occupied[i] == ts) {
	    if (i == NOccupied - 1)
		NOccupied--;
	    else
		Occupied[i] = NULL;
	    break;
	}
    ts->TrainCount--;
    ts->SetOccupied(FALSE);
}
#endif

void Train::SetOccupied (TrackUnit * ts) {
    int i = 0;
    for (i = 0; i < NOccupied; i++)
	if (Occupied[i] == front.ts)
	    return;
    for (i = 0; i < NOccupied; i++)
	if (Occupied[i] == NULL)
	    goto goodi;
    if (i >= MaxOccupied) {
	ts->SetOccupied(TRUE);
	return;
    }
    i = NOccupied++;
goodi:
#ifdef NXV1
    OcTrackTime[i] = ++TrackTime;
#endif
    Occupied[i] = ts;
    ts->TrainCount++;
    ts->SetOccupied(TRUE);
}


Train::~Train () {
    if (CV_TrainID == id)
	CV_TrainID = 0;
    for (int i = 0; i < NOccupied; i++)
	if (Occupied[i] != NULL)
	    if (--Occupied[i]->TrainCount == 0)
		Occupied[i]->SetOccupied(FALSE);
    if (NextSig != NULL)
	NextSig->Hook (NULL);
    KillTimer();
    DestroyWindow (Dialog);
    Trains.erase(id);
    TrainsAlive--;
}

void Train::KillTimer() {
    KillOneTimer (this);
    TimerPending = 0;
}


void Train::Reverse (){
#ifdef NXV1
    southp = !southp;
#endif
    Pointpos temp;
    temp = front;
    front = back;
    back = temp;
#ifdef NXV2
    front.IAmFront = TRUE;
    back.IAmFront = FALSE;
    front.Reverse(0.0f);
    back.Reverse(front.x - Length/100.0);
#endif
    InitPosition();
#ifdef NXV2
    if (NextSig && NextSig->XlkgNo != 0)
	TrySignalIDBox (NextSig->XlkgNo);
#endif
}

int Train::Command (WPARAM cmd) {

    switch (cmd) {
	case TRD_HALT:
	    observant = 0;
	    UpdateSwitches();
	    SetSpeed(0.0);
	    return TRUE;
	case TRD_REV:
	    Reverse();
	    return TRUE;
	case TRD_KILL:
	    delete this;
	    return TRUE;
	case TRD_OBSERVANT:
	    observant = 1;
	    UpdateSwitches();
	    ComputeNextMotion();	/* MUST BE LAST CALL */
	    return TRUE;
	case TRD_DEFIANT:
	    observant = 0;
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
#ifdef NXOGL
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

#ifdef NXSYSMac
void sendTrainSlider(void*v, int of100) {
    Train* t = (Train*) v;
    t->ExplicitSpeed((double)((of100*(t->GetCruise())/100.0)));
}

void sendTrainCommand(void* train, int cmd) {
    ((Train*) train)->Command((WPARAM)cmd);
}
#endif

void Train::ExplicitSpeed (double speed) {
    
    double was_speed = Speed;

    if (speed > Cruise)
	speed = Cruise;
    else if (speed < 0.0)
	speed = 0.0;

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
    for (int j = 0; j < NOccupied; j++)
	if (Occupied[j] == tu) {
	    ShowWindow(Dialog, SW_RESTORE);
	    SetFocus(Dialog);
	    return TRUE;
	}
    return FALSE;
}

extern HWND ChooseTrackDlg;

int FilterTrainDialogMessages (MSG* mp) {
    if (ChooseTrackDlg != NULL)
	if (IsDialogMessage (ChooseTrackDlg, mp))
	    return 1;
    for (decltype(Trains.begin()) it = Trains.begin() ; it != Trains.end(); it++) {
	    if (IsDialogMessage ((it->second)->Dialog, mp))
		return 1;
    }
    return 0;
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
#ifndef NXSYSMac // maybe pretty up later
    HWND reverser = GetDlgItem (Dialog, TRD_REV);
    EnableWindow (reverser, (Speed == 0.0));
#endif
}


void TrainMiscCtl (int cmd) {
    if (cmd == CmKillTrains) {
      //   http://stackoverflow.com/questions/1038708/erase-remove-contents-from-the-map-or-any-other-stl-container-while-iterating
        for (decltype(Trains.begin()) it = Trains.begin() ; it != Trains.end();) {
            Train * t = it->second;
            it ++;
            delete t; // destructor will erase this stl map entry
        }
    }
    else {
        for (decltype(Trains.begin()) it = Trains.begin() ; it != Trains.end(); it++) {
            Train* t = it->second;
            switch (cmd) {
                case CmHaltTrains:
                    t->SetSpeed (0.0);
                    break;
                case CmHideTrainWindows:
                    ShowWindow (t->Dialog, SW_SHOWMINIMIZED);
                    break;
                case CmShowTrainWindows:
                    ShowWindow (t->Dialog, SW_SHOWNORMAL);
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
    if (TrainsAlive == 1)
	X = (int)(.4*dlw);
    else {
	X += (int)(.6*dlw);
	if (X+dlw >= dtw)
	    X = (int)(.2*dlw);
    }
    MoveWindow (Dialog, X, (int)(dth - 1.25*dlh), dlw, dlh, FALSE);
}

#ifdef NXV1
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


int TrainAutoCreate   (int train_no, long id_no, long options) {

/* maybe create by track section number, find loose end? */
    TrackUnit * tk;
#ifdef NXV1
    const char * what = "track number";
    tk = ChooseTrackNumber (id_no);
#endif
#ifdef NXV2
    const char * what = "track end IJ #";
    tk = FindTrainEntryTrackSectionByNomenclature (id_no);
#endif
    if (!tk) {
	usermsgstop ("Invalid train starting %s in demo form: %d", what, id_no);
	return 0;
    }
    Train * t = MakeTrainN (train_no);
    if (!t)
	return 0;
    t->Init(tk, options);
    return t->id;
}

BOOL TrainAutoValidateTrainNo (int train_no) {
    return Trains.count(train_no) > 0;
}

BOOL TrainAutoCmd (int train_no, WPARAM cmd) {
    return TrainAutoValidateTrainNo(train_no)
	    && Trains[train_no]->Command (cmd);
}

BOOL TrainAutoSetSpeed (int train_no, double speed) {
    if (!TrainAutoValidateTrainNo (train_no))
	return NULL;
    Trains[train_no]->ExplicitSpeed (speed);
    return TRUE;
}

double TrainAutoGetSpeed (int train_no) {
    if (!TrainAutoValidateTrainNo(train_no))
	return 0.0;
    else return Trains[train_no]->GetSpeed();
}

BOOL TrainAutoLookupCommand (char * scmd, int&icmd) {
    for (int i = 0; i < N_TrainAutoCmds; i++)
	if (!stricmp (scmd, TrainAutoCmds[i].name)) {
	    icmd = TrainAutoCmds[i].cmd;
	    return TRUE;
	}
    return FALSE;
}

BOOL WindowToTopFromTrackUnit (TrackUnit * tu) {
    for (decltype(Trains.begin()) it = Trains.begin() ; it != Trains.end(); it++) {
        if (it->second->ShowWindowIfOwnTU (tu))
            return TRUE;
    }
    return FALSE;
}
