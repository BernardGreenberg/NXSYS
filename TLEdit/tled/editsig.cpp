#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <cassert>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "signal.h"
#include "xtgtrack.h"
#include "typeid.h"
#include "tledit.h"
#include "undo.h"
#include "resource.h"
#include "assignid.h"
#include "tlpropdlg.h"
#include "WinApiSTL.h"

#define SIGNAL_DUMP_ORDER 200

UINT PanelSignal::DlgId () {return IDD_EDIT_SIGNAL;}

using FUTresult = std::pair<TSAX, TSEX>;

static FUTresult FindUprightTSAX(TrackJoint * tj, bool upright) {
    for (TSAX tsax : {TSAX::IJR0, TSAX::IJR1}) {
        TrackSeg * ts = tj->GetBranch(tsax);
        TSEX endx = ts->FindEndIndex(tj);
        double angle = atan2(ts->SinTheta, ts->CosTheta);
        if (endx == TSEX::E1)
            angle += CONST_PI;
        if (angle > CONST_2PI)
            angle -= CONST_2PI;
        if (angle < 0.0)
            angle += CONST_2PI;
        bool orient_upright = (angle < CONST_PI_OVER_4 || angle >= 5.0*CONST_PI_OVER_4);
        if (upright == orient_upright)
            return FUTresult(tsax,endx);
    }
    /* this ought not happen*/
    assert (!"Can't find an upright TSAX");
    return FUTresult(TSAX::NOTFOUND, TSEX::NOTFOUND);
}

void TLEditCreateSignal (TrackJoint * tj, bool upright) {
    if (tj->TSCount == 3) {
	usererr ("Can't create signal at an actual switch, need an insulated joint.");
	return;
    }

    auto [tsax, endx] = FindUprightTSAX(tj, upright);
    TrackSeg * ts = tj->GetBranch(tsax);
    TrackSegEnd * ep = &ts->GetEnd(endx);

    if (ep->SignalProtectingEntrance != NULL) {  /* TLEdit UI feature */
        ep->SignalProtectingEntrance->PSignal->Select();
        return;
    }

    if (!tj->Insulated) {
        tj->Insulate(true);            /* ensure insulated  ************ NEEDS UNDO HAIR ********/
    }

    /* Commit to create the signal */

    Signal * s = new Signal(0, 0, "GYR");
    ep->SignalProtectingEntrance = s;
    PanelSignal * Ps = new PanelSignal (ts, endx, s, NULL);
    s->TStop = new Stop(s);
    Ps->Select();
    tj->PositionLabel();
    Undo::RecordGOCreation(Ps);
      
    return;
}

void TLEditCreateSignalFromSignal (PanelSignal * ps, bool upright) {
    TrackSeg * ts = ps->Seg;
    TLEditCreateSignal (ts->GetEnd(ps->EndIndex).Joint, upright);
}


HBRUSH Signal::GetGKBrush () {
    if (PSignal->Selected)
	return GKGreenBrush;
    else
	return GKOffBrush;
}

void FlipSignal (PanelSignal * ps) {
    Signal * s = ps->Sig;
    TrackSeg * ts = ps->Seg;
    TSEX end_index = ps->EndIndex;
    TrackSegEnd& E = ts->GetEnd(end_index);
    TrackJoint * tj = E.Joint;
    if (tj->TSCount != 2) {
	usererr ("No place to flip this signal to.");
	return;
    }
    TrackSeg* tsother = (ts == tj->TSA[0]) ? tj->TSA[1] : tj->TSA[0];
    TSEX exother = tsother->FindEndIndex(tj);
    TrackSegEnd& epother = tsother->GetEnd(exother);
    if (epother.SignalProtectingEntrance) {
	usererr ("Both signal positions at this joint are occupied.  Can't flip this signal.");
	return;
    }
//    ps->CacheInitSnapshot();
    E.SignalProtectingEntrance = NULL;
    epother.SignalProtectingEntrance = s;
    ps->Seg = tsother;
    ps->EndIndex = exother;
    ps->Reposition();

    /* There is currently (5-16-2022) no user interface in TLEdit which invokes FlipSignal.
       It's not at all inconceivable to implement undo, but there is no way to test it without
       UI to invoke it, so not worth it right now.  We should call the late Ub Iwerks. */

    Undo::RecordIrreversibleAct("flip signal position.");
//    Undo::RecordChangedProps(ps, ps->StealPropCache());
}

void PanelSignal::Cut () {
    Seg->GetEnd(EndIndex).Joint->Select();
    /* query ++++++++++++++++++++++ */
    Undo::RecordGOCut(this);
    delete this;		/* this destructor deletes the TStop and decouples Seg */
}

void PanelSignal::MakeSelfVisible () {
    GraphicObject::MakeSelfVisible();
    if (Label)
        Label->MakeSelfVisible();
}

Signal::~Signal () {
    if (!NXGODeleteAll) {
	if (XlkgNo)
	    DeAssignID (XlkgNo);
	if (PSignal)
	    PSignal->Seg->GetEnd(PSignal->EndIndex).SignalProtectingEntrance = NULL;
	if (TStop)
	    delete TStop;
    }
}

Signal::Signal (int xlno, int sta_no, const char* headinfo) :
   XlkgNo(xlno), StationNo(sta_no),
   PSignal(nullptr), TStop(nullptr), ExplicitID(false), HeadsString(headinfo) {}


Signal::Signal (int xno, int sno, HeadsArray& headsarray) :
    XlkgNo(xno), StationNo(sno),
    PSignal(nullptr), TStop(nullptr), ExplicitID(false)
{
    for (auto& component : headsarray) {
        if (HeadsString.size())
            HeadsString += ' ';
        HeadsString += component;
    }
}

BOOL PanelSignal::DlgOK(HWND hDlg) {
    BOOL es;

    int old_xlkg_no = Sig->XlkgNo;
    int new_xlkg_no = GetDlgItemInt (hDlg, IDC_EDIT_SIG_LEVER, &es, FALSE);
    if (!es) {
        uerr (hDlg, "Bad Signal lever number.");
        return TRUE;
    }
    
    if (new_xlkg_no != old_xlkg_no)
    {
        if (new_xlkg_no != 0 && !CheckID(new_xlkg_no)) {
            uerr (hDlg, "Lever number %d invalid or already in use.", new_xlkg_no);
            return TRUE;
        }
    }

    int new_sta_no = 0;
    std::string stano_string = GetDlgItemText(hDlg, IDC_EDIT_SIG_STATION_NO);
    if (! stano_string.empty()) {
        new_sta_no = GetDlgItemInt (hDlg, IDC_EDIT_SIG_STATION_NO, &es ,FALSE);
        if (new_sta_no == 0 || !es) {
            uerr (hDlg, "Bad signal Station/Relay ID number: %s", stano_string.c_str());
            return TRUE;
        }
    }

    /* committed to changes at this point */
    ChangeXlkgNo(new_xlkg_no);
    Sig->StationNo = new_sta_no;
    Sig->HeadsString = GetDlgItemText(hDlg, IDC_EDIT_SIG_HEADS);
    SetStoppiness(GetDlgItemCheckState (hDlg, IDC_EDIT_SIG_STOP) != 0);
    Undo::RecordChangedProps(this, StealPropCache());
    EndDialog (hDlg, TRUE);
    return TRUE;

}

void PanelSignal::ChangeXlkgNo(int new_xlkg_no) {
    int old_xlkg_no = Sig->XlkgNo;
    if (new_xlkg_no != old_xlkg_no) {
        if (old_xlkg_no != 0)
            DeAssignID (old_xlkg_no);
        SetXlkgNo (new_xlkg_no, TRUE);
        if (new_xlkg_no != 0)
            MarkIDAssign(new_xlkg_no);
    }
}

void PanelSignal::SetStoppiness(bool has_stop) {
    if (has_stop == (Sig->TStop != NULL))
        return;
    if (has_stop) {
        if (!Sig->TStop) {
            Sig->TStop = new Stop (Sig);
            Sig->TStop->Reposition();
        }
    }
    else {
        delete Sig->TStop;
        Sig->TStop = NULL;
    }
}


BOOL_DLG_PROC_QUAL PanelSignal::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    char buf[64];
    char o;

    switch (message) {
	case WM_INITDIALOG:
	    sprintf (buf, "Signal at IJ %ld",
		      Seg->GetEnd(EndIndex).Joint->Nomenclature);
	    SetDlgItemText (hDlg, IDC_EDIT_SIG_IJID, buf);
	    o = Orientation();
	    if (o == ' ')
		strcpy (buf, "");
	    else
		sprintf (buf, "Orientation: %c", o);
	    SetDlgItemCheckState (hDlg, IDC_EDIT_SIG_STOP, Sig->TStop != NULL);
	    SetDlgItemText (hDlg, IDC_EDIT_SIG_ORIENTATION, buf);
	    if (Sig->StationNo)
		SetDlgItemInt (hDlg, IDC_EDIT_SIG_STATION_NO,
			       Sig->StationNo, FALSE);
	    SetDlgItemInt (hDlg, IDC_EDIT_SIG_LEVER, Sig->XlkgNo, FALSE);
            SetDlgItemText (hDlg, IDC_EDIT_SIG_HEADS, Sig->HeadsString.c_str());
            CacheInitSnapshot();
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
                    return DlgOK(hDlg);
                case IDCANCEL:
                    DiscardPropCache();
		    EndDialog (hDlg, FALSE);
		    return TRUE;		    
		case IDC_EDIT_SIGNAL_JOINT:
		    Seg->GetEnd(EndIndex).Joint->EditProperties();
		    sprintf (buf, "Signal at IJ %ld",
			      Seg->GetEnd(EndIndex).Joint->Nomenclature);
		    SetDlgItemText (hDlg, IDC_EDIT_SIG_IJID, buf);
		default:
		    return FALSE;
	    }
	    return FALSE;
	default:
	    return FALSE;
    }
}

char PanelSignal::Orientation () {

    TrackJoint * tj = Seg->GetEnd(EndIndex).Joint;
    if (tj->TSCount == 1)
	return ' ';

    return Seg->EndOrientationKey (EndIndex);
}


int PanelSignal::Dump (ObjectWriter& W) {
    /* New format!
    (SIGNAL 10101 {  2}  {R/L/T/B} (GYR ...) keywords {ST 20} {GT} {ID 2415}
            IJ id opt  
      {PLATE "E2 415"} {MODEL HOME3} {NOSTOP}...}
    */
    char xlbuf[6];
    char extra [100];
    char idbuf[15];
   // char orient = ' ';
    TrackJoint * tj = Seg->GetEnd(EndIndex).Joint;

    if (Sig->XlkgNo == 0)
	strcpy (xlbuf, "");
    else
	sprintf (xlbuf, "%5d", Sig->XlkgNo);

    if (Sig->TStop)
	strcpy (extra, "");
    else
	strcpy (extra, " NOSTOP");

    if (Sig->StationNo != 0) {
	sprintf (idbuf, " ID %d", Sig->StationNo);
	strcat (extra, idbuf);
    }

    W.putf("  (SIGNAL %6ld %6s %c (%s)%s)\n",
           tj->Nomenclature, xlbuf, Orientation(), Sig->HeadsString.c_str(),
           extra);
    return SIGNAL_DUMP_ORDER;
}

bool PanelSignal::HasManagedID() {
    return true;
}

int PanelSignal::ManagedID() {
    return (int)(Sig->XlkgNo);
}

void PanelSignal::PropCell::Snapshot_(PanelSignal * p) {
    SnapWPpos(p);  /* dlg can't move it, but undo system needs it */
    Signal * S = p->Sig;
    XlkgNo = S->XlkgNo;
    HasStop = S->TStop != nullptr;
    Orientation = p->Orientation();
    StationNo = S->StationNo;
    HeadsString = S->HeadsString;
    Seg = p->Seg;
    SegTSEX = (Seg->Ends[0].SignalProtectingEntrance == S) ? TSEX::E0 : TSEX::E1;
}

void PanelSignal::PropCell::Restore_(PanelSignal * p) {
    Signal * S = p->Sig;
    p->ChangeXlkgNo(XlkgNo);
    S->StationNo = StationNo;
    S->HeadsString = HeadsString;
    p->SetStoppiness(HasStop);
    TrackSeg* seg = p->Seg;
    if (seg != Seg)
        usererr("Signal Prot Entrance Seg changed.");

}


