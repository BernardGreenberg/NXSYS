#include "windows.h"
#include <string.h>
#include <stdio.h>

#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "assignid.h"
#include "trafficlever.h"
#include "resource.h"
#include "tlpropdlg.h"
#include "objreg.h"
#include "nxgo.h"
#include "undo.h"

#include "dragger.h"

static Dragger Dragon;

TrafficLever::~TrafficLever() {
    if (XlkgNo)
        DeAssignID(XlkgNo);
    XlkgNo = 0; /* ihr Kleinglaubigen... */
}

/* There is a different version of this methodfor NXSYS proper in trafficlever.cpp */
void TrafficLever::SetXlkgNo (int xno) {
    if (XlkgNo != xno) {
        if (XlkgNo)
            DeAssignID(XlkgNo);
        XlkgNo = xno;
        NumString = std::to_string(XlkgNo);
        if (XlkgNo)
            MarkIDAssign(XlkgNo);
    }
}

static GraphicObject* CreateTrafficLever (int wpx, int wpy) {
    if (Dragon.Movingp())
	return NULL;
    return Dragon.StartMoving (new TrafficLever (0, wpx, wpy, 0), "New Traffic Lever", G_mainwindow);
}

bool TrafficLever::HasManagedID() {
    return true;
}

int TrafficLever::ManagedID() {
    return (int)XlkgNo;
}

REGISTER_NXTYPE(TypeId::TRAFFICLEVER, CmTrafficLever, IDD_TRAFFICLEVER, CreateTrafficLever, InitTrafficLeverData);

void TrafficLever::EditClick (int x, int y) {
    char d[30];
    sprintf (d, "Traffic Lever %d", XlkgNo);
    Dragon.ClickOn (G_mainwindow, this, d, x, y);
}

int TrafficLever::Dump (ObjectWriter& W) {
    W.putf("  (TRAFFICLEVER 1\t%4d\t%4ld  %4ld %1d)\n",
	 XlkgNo, wp_x, wp_y, NormalIndex);
    return 500;				/* dump order */
}


BOOL_DLG_PROC_QUAL TrafficLever::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL es;

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_TRAFFICLEVER_LEVER, XlkgNo, FALSE);
	    SetDlgItemInt (hDlg, IDC_TRAFFICLEVER_WPX, (int)wp_x, FALSE);
	    SetDlgItemInt (hDlg, IDC_TRAFFICLEVER_WPY, (int)wp_y, FALSE);
	    SetDlgItemCheckState (hDlg, IDC_TRAFFICLEVER_RIGHT, NormalIndex);
	    SetDlgItemCheckState (hDlg, IDC_TRAFFICLEVER_LEFT, ReverseIndex);
            CacheInitSnapshot();
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    /* validate all stuff before changing anything! */
                    WP_cord new_wp_x = GetDlgItemInt (hDlg, IDC_TRAFFICLEVER_WPX, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel X coordinate.");
                        return TRUE;
                    }
                    WP_cord new_wp_y = GetDlgItemInt (hDlg, IDC_TRAFFICLEVER_WPY, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel Y coordinate.");
                        return TRUE;
                    }
                    int newnom = (int)GetDlgItemInt (hDlg, IDC_TRAFFICLEVER_LEVER, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad lever number.");
			return TRUE;
		    }
                    if (newnom != XlkgNo && !CheckID(newnom)) {
                        uerr(hDlg, "Lever number %d is already in use.", newnom);
                        return TRUE;
                    }

                    /* commit; change stuff.*/
                    bool changed = false;
		    if (newnom != XlkgNo) {
                        if (XlkgNo)
                            DeAssignID(XlkgNo);
                        MarkIDAssign(newnom);
			SetXlkgNo(newnom);  /* assigns # */
			StatusMessage ("Traffic Lever %d", newnom);
                        changed = true;
		    }
		    int new_right_normal = GetDlgItemCheckState
					   (hDlg, IDC_TRAFFICLEVER_RIGHT)
					   ? 1 : 0;
		    if (new_right_normal != NormalIndex) {
			SetNormalReverseStatus (new_right_normal);
                        changed = true;
		    }

		    if (wp_x != new_wp_x || wp_y != new_wp_y) {
			MoveWP(new_wp_x, new_wp_y);
                        changed = true;
		    }
                    if (changed) {
                        Invalidate();
                        Undo::RecordChangedProps(this, StealPropCache());
                    }
                    else
                        DiscardPropCache();
		}
		EndDialog (hDlg, TRUE);
		return TRUE;

		case IDCANCEL:
		    EndDialog (hDlg, FALSE);
                    DiscardPropCache();
		    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}

