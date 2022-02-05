/* Purpose of this file is to prevent the OLE stuff from knowing
   anything at all about the semantics or implementation of NXSYS */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

#include "nxgo.h"
#ifdef NXV2

#include "xtgtrack.h"
#include "xturnout.h"
#else
#include "track.h"
#endif
#include "signal.h"
#include "swkey.h"
#include "objid.h"
#include "compat32.h"
#include "trainapi.h"
#include "trainaut.h"
#include "trafficlever.h"
#include "nxsysapp.h"
#include "ssdlg.h"
#include "incexppt.h"
#include "demoapi.h"
#include "loaddcls.h"
#include "lisp.h"
#include "relays.h"
#include "commands.h"


#include "nxoleaut.h"

#define KPRESSDELAY 200

int FindHitSignal (long id, int&x, int&y, int but);
int RelayShowString (char *);

static char DemoPath[_MAX_PATH];

extern unsigned smeasure (HDC dc, char * str);

#ifdef NXV2
#define FINDER FindDemoObjectByID
#else
#define FINDER FindHitObject
#endif

#ifndef NOTRAINS
static struct {
    char * name;
    WPARAM cmd;
} TrainAutoCmds [] = {
    {"OBSERVANT", TRD_OBSERVANT},
    {"FREEWILL",  TRD_DEFIANT},
    {"REVERSE",   TRD_REV},
    {"HALT",      TRD_HALT},
    {"KILL",      TRD_KILL},
    {"CALLON",    TRD_COPB},
    {"MINIMIZE",  TRD_MINIMIZE},
    {"RESTORE",   TRD_RESTORE}
};

static const int N_TrainAutoCmds = sizeof(TrainAutoCmds)/sizeof(TrainAutoCmds[0]);
#endif

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


BOOL CNXAppImp::Load (char * path) {
	std::string B;
    return (BOOL) GetLayout (include_expand_path (DemoPath, path, B), TRUE);
}

BOOL CNXAppImp::Remark (char * text) {
    DemoSay (text);
    return TRUE;
}

BOOL CNXAppImp::Initiate (int signo) {
    Signal * s = CSignalImp::FindSignalByID(signo);
    return s && s->Initiate();
}

BOOL CNXAppImp::CancelAllSignals () {
    DropAllSignals();
    return TRUE;
}

BOOL CNXAppImp::NormalAllSwitches () {
    /* Never forget your colon!!!! :(  */
    ::NormalAllSwitches ();
    return TRUE;
}

BOOL CNXAppImp::KillAllTrains() {
    TrainMiscCtl (CmKillTrains);
    return TRUE;
}

BOOL CNXAppImp::AppWindow (UINT shower) { /* as it were */
    ShowWindow (G_mainwindow, shower);
    return TRUE;
}

BOOL CNXAppImp::ShowStops (int shower) {
    ImplementShowStopPolicy (shower);
    return TRUE;
}

BOOL CNXAppImp::ResetAll () {
    ClearAllTrackSecs();
    DropAllSignals();
    DropAllApproach();
    ClearAllAuxLevers();
    return TRUE;
}

BOOL CNXAppImp::ResetApproachAll () {
    DropAllApproach();
    return TRUE;
}


Signal * CSignalImp::FindSignalByID (long signo) {
#ifdef NXV2
    PanelSignal * P = (PanelSignal *) FindHitObject (signo, ID_SIGNAL);
    if (P)
	return P->Sig;
    else return NULL;
#else
    return (Signal *) FINDER (signo, ID_SIGNAL);
#endif
}


BOOL CSignalImp::Initiate () {
    return S->Initiate();
}

BOOL CSignalImp::Initiated () {
    return S->Initiated();
}

BOOL CSignalImp::Cancel () {
    return S->Cancel();
}

BOOL CSignalImp::CallOn () {
    return S->CallOn();
}

BOOL CSignalImp::InitiateFleet () {
    return S->Fleet(1) && Initiate();
}

BOOL CSignalImp::UnFleet () {
    return S->Fleet(0);
}

BOOL CSignalImp::Aspect (char * buf) {
    char * b = buf;
    S->ComputeState();
    for (int i = 0; i < S->HeadCount; i++) {
	char c = S->Heads[i]->State;
	if (c == 'X')
	    c = ' ';
	*b++ = c;
    }
    *b = '\0';
    return TRUE;
}

BOOL CSignalImp::ShowFullsigWin (BOOL up) {
    return S->ShowFullsigWindow (up);
}

BOOL CSignalImp::ResetApproach () {
    return S->ResetApproach();
}

ExitLight * CExitLightImp::FindExitLightByID (long signo) {
    return (ExitLight *) FINDER (signo, ID_EXITLIGHT);
};

BOOL CExitLightImp::Lit () {
    return E->Lit ? TRUE : FALSE;
}

BOOL CExitLightImp::ClickExit () {
    E->Hit(1);
    return TRUE;
}

TrackCircuitAvatar * CTrackImp::FindTrackByID (long idno){
#ifdef NXV2
    return FindTrackCircuit(idno);
#else
    return (TrackCircuitAvatar *) FINDER (idno, ID_TRACKSEC);
#endif
}

void CTrackImp::SetOccupied(BOOL occupied) {
    T->SetOccupied (occupied);
}

BOOL CTrackImp::Occupied() {
    return T->Occupied;
}

BOOL CTrackImp::Routed() {
    return T->Routed;
};

CSwitchImp::CSwitchImp (SwitchKey * sk) {
    SK = sk;
    T = sk->Turn;
}

SwitchKey * CSwitchImp::FindSwitchKeyByID (long idno){
    return (SwitchKey *) FINDER (idno, ID_SWITCHKEY);
}

BOOL CSwitchImp::Throw(BOOL rev, BOOL hold) {
    BOOL v= SK->Press (rev, hold);
    if (hold)
	return v;
    UpdateWindow (G_mainwindow);
    Sleep (KPRESSDELAY);
    SK->ClearAux();
    UpdateWindow (G_mainwindow);
    return v;
}

BOOL CSwitchImp::SetNormal() {
    return Throw (FALSE, FALSE);
}

BOOL CSwitchImp::SetReverse() {
    return Throw (TRUE, FALSE);
}

BOOL CSwitchImp::SetNormalHold() {
    return SK->Press (FALSE, TRUE);
}

BOOL CSwitchImp::SetReverseHold() {
    return SK->Press (TRUE, TRUE);
}

BOOL CSwitchImp::Normal() {
    return !T->Thrown && !T->MovingPhase;
}

BOOL CSwitchImp::Reverse() {
    return T->Thrown && !T->MovingPhase;
}


TrainId CTrainImp::FindTrainById (int idno) {
    return  (TrainAutoValidateTrainNo (idno)) ? (TrainId)idno : (TrainId)0;
}


TrainId CTrainImp::MakeTrainById (int idno, long birthplace, long options) {
    return (TrainId) TrainAutoCreate (idno, birthplace, options);
}


BOOL CTrainImp::SetSpeed (double spd) {
    if (TrainAutoValidateTrainNo(TrainNo)) {
	TrainAutoSetSpeed (TrainNo, spd);
	return TRUE;
    }
    else return FALSE;
}


BOOL CTrainImp::Command (char * cmd) {
    int icmd;
    return TrainAutoLookupCommand (cmd, icmd)
	    && TrainAutoCmd (TrainNo, icmd);
}


double CTrainImp::GetSpeed () {
    return TrainAutoGetSpeed (TrainNo);
}


BOOL AutomationGetRelayState (char * rnm, BOOL &state) {
    Sexpr s = RlysymFromStringNocreate (rnm);
    if (s == NIL || s.u.r->rly==NULL)
	return FALSE;
    state = (BOOL) s.u.r->rly->State;
    return TRUE;
}


CTrafficLeverImp::CTrafficLeverImp (TrafficLever * tl) {
    TL = tl;
}


TrafficLever * CTrafficLeverImp::FindTrafficLeverByID (long idno){
    return (TrafficLever *) FINDER (idno, ID_TRAFFICLEVER);
}

BOOL CTrafficLeverImp::Throw (BOOL set_reverse) {
    TL->Throw(set_reverse);
    int index = set_reverse ? TL->ReverseIndex : TL->NormalIndex;
    return TL->Indicators[index].White ||TL->Indicators[index].Red;
}

BOOL CTrafficLeverImp::Normal () {
    return !TL->Reverse;
}

BOOL CTrafficLeverImp::Reverse () {
    return TL->Reverse;
}

int  CTrafficLeverImp::NormalLightStatus() {
    if (TL->Indicators[TL->NormalIndex].Red)
	return -1;
    else if (TL->Indicators[TL->NormalIndex].White)
	return 1;
    else return 0;
}


int  CTrafficLeverImp::ReverseLightStatus() {
    if (TL->Indicators[TL->ReverseIndex].Red)
	return -1;
    else if (TL->Indicators[TL->ReverseIndex].White)
	return 1;
    else return 0;
}

