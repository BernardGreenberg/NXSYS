#include "windows.h"
#include <stdio.h>
#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "swkey.h"
#include "resource.h"
#include "tlpropdlg.h"
#include "dragger.h"
#include "objreg.h"
#include "undo.h"

SwitchKey::~SwitchKey() {};
static Dragger Dragon;

GraphicObject* CreateSwKey (int wpx, int wpy) {
    if (Dragon.Movingp())		/* don't create if moving */
	return NULL;
    return Dragon.StartMoving (new SwitchKey (0, wpx, wpy),
			       "New switch key", G_mainwindow);
}

REGISTER_NXTYPE(TypeId::SWITCHKEY, CmAuxKey, IDD_SWKEY, CreateSwKey, InitSwitchKeyData);

void SwitchKey::EditClick (int x, int y) {
    char d[30];
    sprintf (d, "Switch Key %d", XlkgNo);
    Dragon.ClickOn (G_mainwindow, this, d, x, y);
}

int SwitchKey::Dump (ObjectWriter& W) {
    W.putf ("  (SWITCHKEY\t%4d\t%4ld  %4ld)\n", XlkgNo, wp_x, wp_y);
    return 400;
}

BOOL_DLG_PROC_QUAL SwitchKey::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message) {
	case WM_INITDIALOG:
	    SetDlgItemInt (hDlg, IDC_SWKEY_LEVER, XlkgNo, FALSE);
	    SetDlgItemInt (hDlg, IDC_SWKEY_WPX, (int)wp_x, FALSE);
	    SetDlgItemInt (hDlg, IDC_SWKEY_WPY, (int)wp_y, FALSE);
            CacheInitSnapshot();
	    return TRUE;
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
                    BOOL es;
                    WP_cord new_wp_x = GetDlgItemInt (hDlg, IDC_SWKEY_WPX, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel X coordinate.");
                        return TRUE;
                    }
                    WP_cord new_wp_y = GetDlgItemInt (hDlg, IDC_SWKEY_WPY, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel Y coordinate.");
                        return TRUE;
                    }

		    long newnom = GetDlgItemInt (hDlg, IDC_SWKEY_LEVER, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad lever number.");
			return TRUE;
		    }

                    if (!CheckGONumberReuse(hDlg, newnom))
                        return TRUE;
                    
                    bool mod = false;
		    if (newnom != XlkgNo) {
			SetXlkgNo((int)newnom);
			StatusMessage ("Aux switch key %ld", newnom);
                        mod = true;
		    }
                    if (wp_x != new_wp_x || wp_y != new_wp_y) {
                        MoveWP(new_wp_x, new_wp_y);
                    mod = true;
                    }
                    if (mod) {
                        Invalidate();
                        Undo::RecordChangedProps(this, StealPropCache());
                    }
                    else
                        DiscardPropCache();
                }
		EndDialog (hDlg, TRUE);
		return TRUE;

		case IDCANCEL:
                    DiscardPropCache();
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
}

