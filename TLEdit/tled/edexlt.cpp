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
#include "STLExtensions.h"
#include "WinApiSTL.h"

UINT ExitLight::DlgId () {return IDD_EDIT_EXLIGHT;}

void TLEditCreateExitLightFromSignal (PanelSignal* ps, bool upright) {
#ifndef NXSYSMac
    upright;
#endif
    TrackSegEnd& E = ps->Seg->GetEnd(ps->EndIndex);
    ExitLight * xl = E.ExLight;

    if (!xl) {
	BufferModified = TRUE;

	xl = E.ExLight = new ExitLight (ps->Seg, ps->EndIndex, ps->Sig->XlkgNo);
    }

    xl->GetVisible();

    if (ExitLightsShowing)
	xl->SetLit(TRUE);
    xl->Select();
    
}

void ExitLight::Cut () {
    TrackSegEnd & E = Seg->GetEnd(EndIndex);
    delete this;
    if (E.SignalProtectingEntrance)
	E.SignalProtectingEntrance->PSignal->Select();
    else
	E.Joint->Select();
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
	    Seg->GetEnd(EndIndex).SignalProtectingEntrance
	    &&
	    Seg->GetEnd(EndIndex).SignalProtectingEntrance->XlkgNo == XlkgNo)
	    fprintf (f, "  (EXITLIGHT\t%4d)\n", XlkgNo);
	else
	    fprintf (f, "  (EXITLIGHT\t%4d %c %5d)\n",
		     XlkgNo,
		     Seg->EndOrientationKey(EndIndex),
		     (int)Seg->GetEnd(EndIndex).Joint->Nomenclature);
    }
    return 300;				/* dump order */
}


BOOL_DLG_PROC_QUAL ExitLight::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL es;
    char o;

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_EDIT_XLIGHT_XLNO, XlkgNo, FALSE);
	    SetDlgItemTextS (hDlg, IDC_EDIT_XLIGHT_IJNO,
                            FormatString("Exit Light at IJ %ld",
                                         Seg->GetEnd(EndIndex).Joint->Nomenclature));

	    o = Seg->EndOrientationKey(EndIndex);
	    SetDlgItemTextS (hDlg, IDC_EDIT_XLIGHT_ORIENT,
                            FormatString("Orientation: %c", o));
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
