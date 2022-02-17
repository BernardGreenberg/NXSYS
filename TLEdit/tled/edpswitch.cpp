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

static Dragger Dragon;

PanelSwitch::~PanelSwitch() {}


static GraphicObject* CreatePanelSwitch (int wpx, int wpy) {
    if (Dragon.Movingp())
	return NULL;
    return Dragon.StartMoving (new PanelSwitch (0, wpx, wpy, ""), "New Panel Switch", G_mainwindow);
}

REGISTER_NXTYPE(ID_PANELSWITCH, CmPanelSwitch, IDD_PANELSWITCH, CreatePanelSwitch, InitPanelSwitchData);

void PanelSwitch::EditClick (int x, int y) {
    char d[30];
    sprintf (d, "Panel Switch %d", XlkgNo);
    Dragon.ClickOn (G_mainwindow, this, d, x, y);
}

int PanelSwitch::Dump (FILE * f) {
    if (RelayNomenclature.size() == 0) {
        RelayNomenclature = "PNLSW";
    }
    if (f != NULL)  {
	fprintf (f, "  (PANELSWITCH %d %d %d\t%s\t%4ld %4ld %s)\n",
		 1, XlkgNo, 2, "\"\"", wp_x, wp_y, RelayNomenclature.c_str());
    }
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
	    return TRUE;
	}
	case WM_COMMAND:
	    switch (wParam) {
		case IDOK:
		{
		    long newnom = GetDlgItemInt (hDlg, IDC_PANELSWITCH_LEVER, &es, FALSE);
		    if (!es) {
			uerr (hDlg, "Bad lever number.");
			return TRUE;
		    }

		    if (newnom != XlkgNo) {
			SetXlkgNo((int)newnom);
			StatusMessage ("Panel switch/%ld", newnom);
			Invalidate();
			BufferModified = TRUE;
		    }

		    char * newnomen = GetPSDlgString (hDlg, IDC_PANELSWITCH_NOMENCLATURE);
                    if (RelayNomenclature != newnomen) {
			BufferModified = TRUE;
                        RelayNomenclature = newnomen;
		    }
		}


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
		    if (wp_x != new_wp_x || wp_y != new_wp_y) {
			MoveWP(new_wp_x, new_wp_y);
			BufferModified = TRUE;
		    }
		}
		EndDialog (hDlg, TRUE);
		return TRUE;

		case IDCANCEL:
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

