#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nxgo.h"
#include "xtgtrack.h"
#include "tledit.h"
#include "pswitch.h"
#include "resource.h"
#include "tlpropdlg.h"
#include "objreg.h"

#include "dragger.h"
#include "undo.h"

static Dragger Dragon;

PanelSwitch::~PanelSwitch() {}


static GraphicObject* CreatePanelSwitch (int wpx, int wpy) {
    if (Dragon.Movingp())
	return NULL;
    return Dragon.StartMoving (new PanelSwitch (0, wpx, wpy, ""), "New Panel Switch", G_mainwindow);
}

REGISTER_NXTYPE(TypeId::PANELSWITCH, CmPanelSwitch, IDD_PANELSWITCH, CreatePanelSwitch, InitPanelSwitchData);

void PanelSwitch::EditClick (int x, int y) {
    Dragon.ClickOnWNum (G_mainwindow, this, "Panel Switch", XlkgNo, x, y);
}

int PanelSwitch::Dump (ObjectWriter& W) {
    if (RelayNomenclature.size() == 0) {
        RelayNomenclature = "PNLSW";
    }
    W.putf("  (PANELSWITCH %d %d %d\t%s\t%4ld %4ld %s)\n",
	 1, XlkgNo, 2, "\"\"", wp_x, wp_y, RelayNomenclature.c_str());
    return 523;				/* dump order */
}


static char * GetPSDlgString (HWND hDlg, UINT id) {
    static char buf[30];
    GetDlgItemText (hDlg, id, buf, sizeof(buf));
    if (buf[strspn (buf, " ")] == '\0')
	buf[0] = '\0';
    return buf;
}



BOOL_DLG_PROC_QUAL PanelSwitch::DlgProc  (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    BOOL es;
    switch (message) {
	case WM_INITDIALOG:
	{
	    SetDlgItemInt  (hDlg, IDC_PANELSWITCH_LEVER, XlkgNo, FALSE);
	    SetDlgItemInt  (hDlg, IDC_PANELSWITCH_WPX, (int)wp_x, FALSE);
	    SetDlgItemInt  (hDlg, IDC_PANELSWITCH_WPY, (int)wp_y, FALSE);
	    SetDlgItemText (hDlg, IDC_PANELSWITCH_NOMENCLATURE, RelayNomenclature.c_str());
            CacheInitSnapshot();
	    return TRUE;
	}
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
                    WP_cord new_wp_x = GetDlgItemInt (hDlg, IDC_PANELSWITCH_WPX, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel X coordinate.");
                        return TRUE;
                    }
                    WP_cord new_wp_y = GetDlgItemInt (hDlg, IDC_PANELSWITCH_WPY, &es, FALSE);
                    if (!es) {
                        uerr (hDlg, "Bad number in Panel Y coordinate.");
                        return TRUE;
                    }

		    long newnom = GetDlgItemInt (hDlg, IDC_PANELSWITCH_LEVER, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad lever number.");
			return TRUE;
		    }
                    
                    if (!CheckGONumberReuse(hDlg, newnom))
                        return TRUE;

                    bool changed = false;
		    if (newnom != XlkgNo) {
			SetXlkgNo((int)newnom);
			StatusMessage ("Panel switch/%ld", newnom);
                        changed = true;
		    }

		    char * newnomen = GetPSDlgString (hDlg, IDC_PANELSWITCH_NOMENCLATURE);
                    if (RelayNomenclature != newnomen) {
                        RelayNomenclature = newnomen;
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
                    DiscardPropCache();
		    EndDialog (hDlg, FALSE);
		    return TRUE;
		default:
		    return FALSE;
	    }
	default:
	    return FALSE;
    }
    return FALSE;
}

