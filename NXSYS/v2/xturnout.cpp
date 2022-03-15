#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "compat32.h"
#include "nxgo.h"
#include "timers.h"
#include <math.h>

#include "objid.h"

#include "xtgtrack.h"
#include "xturnout.h"

#ifdef REALLY_NXSYS
#include "resource.h"
#include "rlymenu.h"
#include "nxsysapp.h"
#endif

static Turnout** AllTurnouts = NULL;
static int NTurnouts = 0;
int meterNTurnouts () {return NTurnouts;}
Turnout::Turnout (int xno) {
    XlkgNo = xno;
    MovingPhase = CLKCodingPhase = Thrown = 0;
    LockSafe = 1;
    NWP = RWP = NULL;
    NWZ = RWZ = NL = RL = NULL;
    CLK_Coding = 0;
    NEnds = 0;
    AuxKeyForce = 0;
    Joints[0] = NULL;
    Joints[1] = NULL;
    for (int i = 0; i < 2; i++)
	for (int j = 0; j < 2; j++){
	    NK[i][j].ExLight = NULL;
	    NK[i][j].Circuit = NULL;
	    NK[i][j].Status = 0;
	}
}

void Turnout::SegConflict (Turnout* tn) {
    /* not really great to deal with here -- */

#if 0
    char * ab0 = " AB";
    char buf[150];
    wsprintf (buf,
	      "Seg conflict between turnout %d%c and %d",
	      tj->Nomenclature, ab0[tj->SwitchAB0],
	      ts->OwningTurnout->XlkgNo);
    MessageBox (0, buf, "XTG loader", MB_OK | MB_ICONSTOP);
#endif
}

int Turnout::AssignJoint (TrackJoint * tj) {
    int ab0 = tj->SwitchAB0;
    if (ab0 == 0)
	ab0 = 1;
    ab0 --;				/* now 0 or 1 */
    Joints[ab0] = tj;
    if (ab0 == 1)
	NEnds = 2;
    else if (NEnds == 0)
	NEnds = 1;
    tj->TurnOut = this;
    for (int i = (int)TSAX::MIN; i <= (int)TSAX::MAX; i++) {
        TSAX ibx = (TSAX)i;
	int nkx = 0;
	TrackSeg * ts = tj->TSA[i];
	TSEX endx = ts->FindEndIndex(tj);
	TrackSegEnd& E = ts->GetEnd(endx);
	if (!E.ExLight)
	    E.ExLight = new ExitLight (ts, endx, 0);
	switch (ibx) {
            case TSAX::STEM:
		E.FacingSwitch = this;
		E.NextIfSwitchThrown = tj->GetBranch(TSAX::REVERSE);
                E.EndIndexReverse = E.NextIfSwitchThrown->FindEndIndex(tj);
                E.Next = tj->GetBranch(TSAX::NORMAL);

		break;

            case TSAX::REVERSE:
		if (ts->OwningTurnout)
		    /* slip switches'll do that to ya .... */
		    SegConflict (ts->OwningTurnout);
		else
		    ts->OwningTurnout = this;
                E.Next = tj->GetBranch(TSAX::STEM);
		
		nkx = NKX_R;
		break;

            case TSAX::NORMAL:
		E.Next = tj->GetBranch(TSAX::STEM);
		nkx = NKX_N;
		break;
            case TSAX::NOTFOUND:
                assert ("!TSAX::NOTFOUND in turnout creation.");
	}
	if (ibx != TSAX::STEM) {
	    NK[ab0][nkx].ExLight = E.ExLight;
	    NK[ab0][nkx].Circuit = ts->Circuit;
	    NK[ab0][nkx].Status = 0;
	}
	E.EndIndexNormal = E.Next->FindEndIndex(tj);

    }
    return 1;
}

int CreateTurnouts (TrackJoint ** joints, int njoints) {
    if (AllTurnouts)
	delete []AllTurnouts;
    NTurnouts = njoints;
    AllTurnouts = new Turnout*[NTurnouts];
    for (int i = 0; i < njoints; i++) {
	TrackJoint * tj = joints[i];
	if (tj->TurnOut == NULL) {
	    Turnout * tn = new Turnout ((int)tj->Nomenclature);
	    if (!tn->AssignJoint(tj))
		return 0;
	    for (int j = 0; j < njoints; j++) {
		TrackJoint * tj2 = joints[j];
		if (tj2 != tj && tj2->Nomenclature == tj->Nomenclature)
		    if (!tn->AssignJoint (tj2))
			return 0;
	    }
	}
    }
    for (int i = 0; i < njoints; i++) {
	TrackJoint * tj = joints[i];
	if (tj->TurnOut) {
	    AllTurnouts[i] = tj->TurnOut;
	    tj->TurnOut->UpdateRoutings();
	}
    }

    return 1;	 /* success */
}


void Turnout::InvalidateAndTurnouts () {
    for (int ab0 = 0; ab0 < 2; ab0++)
	for (int nr = 0; nr < 2; nr ++) {
	    NKData * np = &NK[ab0][nr];
	    ExitLight * xl = np->ExLight;
	    if (xl == NULL)
		continue;
	    int thrownx = Thrown ? NKX_R : NKX_N;

	    BOOL lol = np->Circuit
		       && (np->Circuit->Occupied || np->Circuit->Routed);
#if 0
	    if (lol) {
		if (np->Status) {
		    np->Status = 0;
		    xl->Seg->Invalidate();
		}
		continue;
	    }
#endif
	    BOOL moving = (MovingPhase != 0) && !lol;
	    BOOL steady_white
		    = (thrownx == nr) &&
		      ((nr == NKX_N) ?
		       (AuxKeyForce == TN_AUXKEY_FORCE_NORMAL) :
		       (AuxKeyForce == TN_AUXKEY_FORCE_REVERSE))
		      && !lol && !moving;
	    BOOL flashing_white = (thrownx == nr) &&
				  (CLKCodingPhase != 0)
				  && !lol && !moving && !steady_white;

	    if (nr == NKX_N)
		moving = FALSE;

	    int newstat = 0;
	    if (moving) newstat= NKS_FlashingRed;
	    if (steady_white) newstat = NKS_IndicatingWhite;
	    if (flashing_white) newstat = NKS_FlashingWhite;
	    if (lol) newstat |= NKS_LineOfLite;

	    if (np->Status != newstat) {
		if (np->Status & NKS_Any) {
		    xl->SetLit(FALSE);
		    xl->SetRedFlash(FALSE);
		    if (lol)
			xl->Seg->Invalidate();
		}
	    }
	    np->Status = newstat;
	    if (steady_white)
		xl->SetLit(TRUE);
	    else if (flashing_white)
		xl->SetLit (CLKCodingPhase == 2);
	    else if (moving)
		xl->SetRedFlash (MovingPhase == 2);
	}
}

void Turnout::DisplayNoDC () {
    /* +++++++++++++++++++  do better, find xl's display them */
    InvalidateAndTurnouts();
}

void Turnout::EditContextMenu (HMENU m) {
    if (!MovingPhase && !Thrown)
	EnableMenuItem (m, ID_NORMAL, MF_BYCOMMAND|MF_GRAYED);
    if (!MovingPhase && Thrown)
	EnableMenuItem(m, ID_REVERSE, MF_BYCOMMAND|MF_GRAYED);
}

void Turnout::Hit (int mousewhat, TrackSeg* ht) {
    if (mousewhat == WM_RBUTTONDOWN)
	ThrowSwitch (!Thrown);
#ifdef REALLY_NXSYS
    else if (mousewhat == WM_NXGO_RBUTTONCONTROL) {
	switch (ht->RunContextMenu (IDR_SWITCH_CONTEXT_MENU)) {
	    case ID_NORMAL:
		ThrowSwitch (FALSE);
		break;
	    case ID_REVERSE:
		ThrowSwitch (TRUE);
		break;
	    case ID_DRAW_RELAY:
		DrawRelaysForObject (XlkgNo, "Switch");
		break;
	    case ID_RELAY_QUERY:
		ShowStateRelaysForObject (XlkgNo, "Switch");
		break;
	    default:
		break; /* whoops, no pointer */
	}
    }
#endif
}

void Turnout::UpdateRoutings () {
    for (int i = 0; i < NEnds; i++) {
	TrackJoint * tj = Joints[i];
	TrackCircuit * circuit = tj->GetBranch(TSAX::STEM)->Circuit;
	if (circuit && circuit->MultipleSegmentsP())
	    circuit->ComputeSwitchRoutedState();
    }
}

#ifdef REALLY_NXSYS

void NormalAllSwitches () {
    for (int i = 0; i < NTurnouts; i++) {
	Turnout * tn = AllTurnouts [i];
	if (tn && tn->Thrown)
	    tn->ThrowSwitch(0);
    }
}

GraphicObject * FindDemoHitTurnout (long id) {
    for (int j = 0; j < NTurnouts; j++)
	if (AllTurnouts [j]->XlkgNo == id)
	    return AllTurnouts[j]->GetOwningSeg();
    return NULL;
}

TrackSeg * Turnout::GetOwningSeg() {
    TrackJoint * tj = Joints[0];
    return tj->GetBranch(TSAX::REVERSE);
}


#endif


