#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "signal.h"
#include "xtgtrack.h"
#include "objid.h"
#include "tledit.h"
#include "resource.h"
#include "assignid.h"
#include "tlpropdlg.h"

#define SIGNAL_DUMP_ORDER 200

UINT PanelSignal::DlgId () {return IDD_EDIT_SIGNAL;}

void TLEditCreateSignal (TrackJoint * tj, bool upright) {
    if (tj->TSCount == 3) {
	usererr ("Can't create signal at an actual switch, need an insulated joint.");
	return;
    }

    tj->Insulate();			/* ensure insulated */
    for (int i = 0; i < tj->TSCount; i++) {
	TrackSeg * ts = tj->TSA[i];
	int ex = tj->FindEndIndex(ts);
	TrackSegEnd * ep = &ts->Ends[ex];
	double angle = atan2(ts->SinTheta, ts->CosTheta);
	if (ex == 1)
	    angle += CONST_PI;
	if (angle > CONST_2PI)
	    angle -= CONST_2PI;
	if (angle < 0.0)
	    angle += CONST_2PI;
	bool orient_upright
		= (angle < CONST_PI_OVER_4 || angle >= 5.0*CONST_PI_OVER_4);
	if (upright == orient_upright) {
	    if (ep->SignalProtectingEntrance == NULL) {
		Signal * s = new Signal;
		ep->SignalProtectingEntrance = s;
		PanelSignal * Ps = new PanelSignal (ts, ex, s, NULL);
		s->TStop = new Stop(s);
		Ps->Select();
		tj->PositionLabel();
		BufferModified = TRUE;
	    }
	    else
		ep->SignalProtectingEntrance->PSignal->Select();
	    return;
	}
    }

}

void TLEditCreateSignalFromSignal (PanelSignal * ps, bool upright) {
    TrackSeg * ts = ps->Seg;
    int ex = ps->EndIndex;
    TLEditCreateSignal (ts->Ends[ex].Joint, upright);
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
    int end_index = ps->EndIndex;
    TrackSegEnd * ep = &ts->Ends[end_index];
    TrackJoint * tj = ep->Joint;
    if (tj->TSCount != 2) {
	usererr ("No place to flip this signal to.");
	return;
    }
    TrackSeg* tsother = (ts == tj->TSA[0]) ? tj->TSA[1] : tj->TSA[0];
    int exother = tj->FindEndIndex(tsother);
    TrackSegEnd *epother = &tsother->Ends[exother];
    if (epother->SignalProtectingEntrance) {
	usererr ("Both signal positions at this joint are occupied.  Can't flip this signal.");
	return;
    }
    ep->SignalProtectingEntrance = NULL;
    epother->SignalProtectingEntrance = s;
    ps->Seg = tsother;
    ps->EndIndex = exother;
    ps->Reposition();
    BufferModified = TRUE;
}

void PanelSignal::Cut () {
    Seg->Ends[EndIndex].Joint->Select();
    /* query ++++++++++++++++++++++ */
    BufferModified = TRUE;
    delete this;		/* should del Sig, and fix seg */
}

Signal::~Signal () {
    if (!NXGODeleteAll) {
	if (XlkgNo)
	    DeAssignID (XlkgNo);
	if (PSignal)
	    PSignal->Seg->Ends[PSignal->EndIndex].SignalProtectingEntrance = NULL;
	if (TStop)
	    delete TStop;
    }
    if (HeadsString)
	free((void*)HeadsString);
}


Signal::Signal () {
    XlkgNo = 0;
    StationNo = 0;
    HeadsString = strdup("GYR");
    PSignal = NULL;
    TStop = NULL;
    ExplicitID = FALSE;
}


BOOL PanelSignal::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    char buf[64];
    BOOL es;
    char o;

    switch (message) {
	case WM_INITDIALOG:
	    sprintf (buf, "Signal at IJ %ld",
		      Seg->Ends[EndIndex].Joint->Nomenclature);
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
	    SetDlgItemText (hDlg, IDC_EDIT_SIG_HEADS, Sig->HeadsString);
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    int newsno = Sig->StationNo;
		    int oldno = Sig->XlkgNo;
		    int newno = GetDlgItemInt (hDlg, IDC_EDIT_SIG_LEVER,
					       &es ,FALSE);
		    if (!es) {
			uerr (hDlg, "Bad Signal lever number.");
			return TRUE;
		    }
		    if (newno != oldno) {
			if (newno != 0 && !CheckID(newno)) {
			    uerr (hDlg, "Lever number %d invalid or already in use.",
				  newno);
			    return TRUE;
			}
		    }
		    if (oldno != 0) DeAssignID (oldno);
		    GetDlgItemText (hDlg, IDC_EDIT_SIG_STATION_NO,
				    buf, sizeof(buf) -1);
		    const char * stripp = buf;
		    while (isspace((char unsigned) *stripp)) stripp++;
		    if (*stripp == 0)
			newsno = 0;
		    else {
			newsno = GetDlgItemInt (hDlg,
						IDC_EDIT_SIG_STATION_NO,
						&es ,FALSE);
			if (newsno == 0 || !es) {
			    uerr (hDlg,
				  "Bad signal Station/Relay ID number: %s",
				  stripp);
			    return TRUE;
			}
		    }

		    Sig->StationNo = newsno;
		    SetXlkgNo (newno, TRUE);
		    MarkIDAssign(newno);
		    EndDialog (hDlg, TRUE);
		    GetDlgItemText (hDlg, IDC_EDIT_SIG_HEADS, buf, sizeof(buf) -1);
		    if (!!strcmp (buf, Sig->HeadsString)) {
			free ((void *)Sig->HeadsString);
			Sig->HeadsString = strdup (buf);
		    }
		    int has_stop = GetDlgItemCheckState (hDlg, IDC_EDIT_SIG_STOP);
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
		    BufferModified = TRUE;
		    return TRUE;
		}
		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;		    
		case IDC_EDIT_SIGNAL_JOINT:
		    Seg->Ends[EndIndex].Joint->EditProperties();
		    sprintf (buf, "Signal at IJ %ld",
			      Seg->Ends[EndIndex].Joint->Nomenclature);
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

    TrackJoint * tj = Seg->Ends[EndIndex].Joint;
    if (tj->TSCount == 1)
	return ' ';

    return Seg->EndOrientationKey (EndIndex);
}


int PanelSignal::Dump (FILE * f) {
    if (f == NULL)
	return SIGNAL_DUMP_ORDER;
    /* New format!
    (SIGNAL 10101 {  2}  {R/L/T/B} (GYR ...) keywords {ST 20} {GT} {ID 2415}
            IJ id opt  
      {PLATE "E2 415"} {MODEL HOME3} {NOSTOP}...}
    */
    char xlbuf[6];
    char extra [100];
    char idbuf[15];
   // char orient = ' ';
    TrackJoint * tj = Seg->Ends[EndIndex].Joint;

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

    fprintf (f, "  (SIGNAL %6ld %6s %c (%s)%s)\n",
	     tj->Nomenclature, xlbuf, Orientation(), Sig->HeadsString,
	     extra);
    return SIGNAL_DUMP_ORDER;
}
