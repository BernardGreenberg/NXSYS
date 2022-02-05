#include "windows.h"
#include <stdio.h>

#include "compat32.h"
#include "nxgo.h"
#include "brushpen.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "tlpropdlg.h"
#include "resource.h"
#include "signal.h"

UINT ExitLight::DlgId () {return IDD_EDIT_EXLIGHT;}

void TLEditCreateExitLightFromSignal (PanelSignal* ps, bool upright) {
#ifndef NXSYSMac
    upright;
#endif
    TrackSegEnd * ep = &ps->Seg->Ends[ps->EndIndex];
    ExitLight * xl = ep->ExLight;

    if (!xl) {
	BufferModified = TRUE;

	xl = ep->ExLight = new ExitLight (ps->Seg, ps->EndIndex, ps->Sig->XlkgNo);
    }

    xl->GetVisible();

    if (ExitLightsShowing)
	xl->SetLit(TRUE);
    xl->Select();
    
}

void ExitLight::Cut () {
    TrackSegEnd * ep = &Seg->Ends[EndIndex];
    delete this;
    if (ep->SignalProtectingEntrance)
	ep->SignalProtectingEntrance->PSignal->Select();
    else
	ep->Joint->Select();
    BufferModified = TRUE;
}

void ExitLight::Select () {
    GraphicObject::Select();
    StatusMessage ("Exit light #%ld", XlkgNo);
}


int ExitLight::Dump (FILE * f) {
    if (f) {
	if (XlkgNo != 0L
	    &&
	    Seg->Ends[EndIndex].SignalProtectingEntrance
	    &&
	    Seg->Ends[EndIndex].SignalProtectingEntrance->XlkgNo == XlkgNo)
	    fprintf (f, "  (EXITLIGHT\t%4d)\n", XlkgNo);
	else
	    fprintf (f, "  (EXITLIGHT\t%4d %c %5d)\n",
		     XlkgNo,
		     Seg->EndOrientationKey(EndIndex),
		     (int)Seg->Ends[EndIndex].Joint->Nomenclature);
    }
    return 300;				/* dump order */
}


BOOL ExitLight::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL es;
    char buf[50];
    char o;

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_EDIT_XLIGHT_XLNO, XlkgNo, FALSE);
	    sprintf (buf, "Exit Light at IJ %ld",
		      Seg->Ends[EndIndex].Joint->Nomenclature);
	    SetDlgItemText (hDlg, IDC_EDIT_XLIGHT_IJNO, buf);
	    o = Seg->EndOrientationKey(EndIndex);
	    sprintf (buf, "Orientation: %c", o);
	    SetDlgItemText (hDlg, IDC_EDIT_XLIGHT_ORIENT, buf);
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		case IDOK:
		{
		    int newno = GetDlgItemInt (hDlg, IDC_EDIT_XLIGHT_XLNO,
					       &es ,FALSE);
		    if (!es) {
			uerr (hDlg, "Bad ExitLight lever number.");
			return TRUE;
		    }
		    if (newno != XlkgNo) {
			XlkgNo = newno;
			BufferModified = TRUE;
			Select();	/* cause new status line */
		    }
		    EndDialog (hDlg, TRUE);
		    return TRUE;
		}
	    }
	default:
	    return FALSE;
    }
}
