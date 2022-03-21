#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "compat32.h"
#include "nxgo.h"
#include "nxsysapp.h"
#include "rlyapi.h"

#include "timers.h"
#include <math.h>
#include "rlyapi.h"
#include "xtgtrack.h"
#include "xturnout.h"

const int SwitchMoveTimeMS = 1800;

void Turnout::LSReporter(BOOL state, void* v) {
    ((Turnout *) v)->LockSafe = state;
}

void Turnout::TimeReporter (void* v) {
    ((Turnout *) v)->Timer();
}

void Turnout::RWZReporter (BOOL state, void * v) {
    Turnout * t = (Turnout *) v;
    if (state && !t->Thrown)
	t->StartMove();
}

void Turnout::NWZReporter (BOOL state, void * v) {
    Turnout * t = (Turnout *) v;
    if (state && t->Thrown)
	t->StartMove();
}

void Turnout::CoderReporter(void* v, BOOL codestate) {
    Turnout * t = (Turnout *) v;
    t->CodingFlash (codestate);
}

void Turnout::StartMove() {
    if (MovingPhase > 0)
	return;
    MovingPhase = 1;
    ReportToRelay (RWP, FALSE);
    ReportToRelay (NWP, FALSE);
    MoveStartTime = GetTickCount();
    NXTimer (this, TimeReporter, SwitchMoveTimeMS);
    NXCoder (this, CoderReporter);
    InvalidateAndTurnouts();
}


void Turnout::ThrowSwitch (int throwit) {
    /* Names "Throw" and "throw" are all taken already by C/Win */

    if (RelayUseDefined (NL) && RelayUseDefined (RL))	 /* new way is best */
	if (MovingPhase == 0)		/* gimme a break! */
	    PulseToRelay (throwit ? RL : NL);
	else;
    else
	if (LockSafe) {
	    ReportToRelay (throwit ? NWZ : RWZ, FALSE);
	    ReportToRelay (throwit ? RWZ : NWZ, TRUE);
	}
}


void Turnout::CodingFlash(BOOL state) {
    if (MovingPhase > 0)
	MovingPhase = state ? 1 : 2;
    else
	if (CLKCodingPhase > 0)
	    CLKCodingPhase = state ? 1 : 2;
    DisplayNoDC();
}


void Turnout::CLKReport(BOOL state) {
    if (state) {
	CLK_Coding = 1;
	if (CLKCodingPhase == 0) {
	    NXCoder (this, CoderReporter);
	    CLKCodingPhase = 2;
	}
    }
    else {
	if (MovingPhase == 0)
	    KillOneCoder (this);


	CLKCodingPhase = 0;
	DisplayNoDC();
	CLK_Coding = 0;	
    }
    InvalidateAndTurnouts();
}

void Turnout::CLKReporter(BOOL state, void * v) {
    Turnout * t = (Turnout *) v;
    t->CLKReport(state);
}

void Turnout::Timer () {
    MovingPhase = 0;
    if (CLKCodingPhase == 0)
	KillOneCoder (this);
    Thrown = !Thrown;

    if ((Thrown && RelayState(NWZ)) || (!Thrown && RelayState(RWZ)))
	StartMove();
    else {
	ReportToRelay (Thrown ? RWP : NWP, TRUE);
	UpdateRoutings();
	InvalidateAndTurnouts();
    }
}
